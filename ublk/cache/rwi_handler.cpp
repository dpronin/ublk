#include "rwi_handler.hpp"

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <memory>
#include <utility>

#include <gsl/assert>

#include "utils/algo.hpp"
#include "utils/utility.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::cache {

RWIHandler::RWIHandler(
    std::shared_ptr<cache::flat_lru<uint64_t, std::byte>> cache,
    std::shared_ptr<IRWHandler> handler,
    std::shared_ptr<mm::mem_chunk_pool> mem_chunk_pool) noexcept
    : cache_(std::move(cache)), handler_(std::move(handler)),
      mem_chunk_pool_(std::move(mem_chunk_pool)), last_wq_done_seq_(0) {
  Ensures(cache_);
  Ensures(handler_);
  Ensures(mem_chunk_pool_);
}

int RWIHandler::submit(std::shared_ptr<read_query> rq) noexcept {
  Expects(rq);
  Expects(0 != rq->buf().size());

  auto chunk_id{rq->offset() / cache_->item_sz()};

  for (size_t rb{0}; rb < rq->buf().size();) {
    auto const chunk_offset{(rq->offset() + rb) % cache_->item_sz()};

    auto const chunk_sz{
        std::min(cache_->item_sz() - chunk_offset, rq->buf().size() - rb),
    };
    auto const chunk{rq->buf().subspan(rb, chunk_sz)};

    if (auto cached_chunk = cache_->find(chunk_id); !cached_chunk.empty())
        [[likely]] {
      auto const from{cached_chunk.subspan(chunk_offset, chunk.size())};
      auto const to{chunk};
      algo::copy(from, to);
    } else {
      auto mem_chunk = mem_chunk_pool_->get();
      Ensures(mem_chunk);

      auto mem_chunk_span = mem_chunk_pool_->chunk_view(mem_chunk);

      auto chunk_rq = read_query::create(
          mem_chunk_span, chunk_id * cache_->item_sz(),
          [=, this, last_wq_done_seq = last_wq_done_seq_,
           mem_chunk_holder = std::make_shared<decltype(mem_chunk)>(
               std::move(mem_chunk))](read_query const &new_rq) mutable {
            if (new_rq.err()) [[unlikely]] {
              rq->set_err(new_rq.err());
              return;
            }

            auto const from{new_rq.buf().subspan(chunk_offset, chunk.size())};
            auto const to{chunk};
            algo::copy(from, to);

            if (last_wq_done_seq == last_wq_done_seq_)
              cache_->update({chunk_id, std::move(*mem_chunk_holder)});
          });

      if (auto const res{handler_->submit(std::move(chunk_rq))}) [[unlikely]] {
        return res;
      }
    }

    ++chunk_id;
    rb += chunk.size();
  }

  return 0;
}

int RWIHandler::submit(std::shared_ptr<write_query> wq) noexcept {
  Expects(wq);
  Expects(0 != wq->buf().size());

  auto const chunk_id_first{wq->offset() / cache_->item_sz()};
  auto const chunk_id_last{
      div_round_up(wq->offset() + wq->buf().size(), cache_->item_sz()),
  };

  auto new_wq{
      wq->subquery(0, wq->buf().size(), wq->offset(),
                   [=, this, wq = std::move(wq)](write_query const &chunk_wq) {
                     cache_->invalidate_range({chunk_id_first, chunk_id_last});
                     ++last_wq_done_seq_;
                     if (chunk_wq.err()) [[unlikely]] {
                       wq->set_err(chunk_wq.err());
                       return;
                     }
                   }),
  };

  if (auto const res{handler_->submit(std::move(new_wq))}) [[unlikely]] {
    return res;
  }

  return 0;
}

} // namespace ublk::cache
