#include "backend.hpp"

#include <algorithm>
#include <ranges>
#include <span>
#include <utility>

#include <gsl/assert>

#include <range/v3/view/concat.hpp>

#include "mm/mem.hpp"

namespace {

auto handlers_data_view(
    std::vector<std::shared_ptr<ublk::IRWHandler>> const &hs,
    uint64_t hid_to_skip, size_t hid_first, size_t hs_nr) noexcept {
  Expects(hid_to_skip < hs.size());
  Expects(hid_first + hs_nr < hs.size());

  auto const hs_first_part{std::span{hs.cbegin(), hid_to_skip}};
  auto const hs_last_part{std::span{hs.cbegin() + hid_to_skip + 1, hs.cend()}};

  /* clang-format off */
  return   ranges::views::concat(hs_first_part, hs_last_part)
         | std::views::drop(hid_first)
         | std::views::take(hs_nr);
  /* clang-format on */
}

} // namespace

namespace ublk::raidsp {

backend::backend(
    uint64_t strip_sz, std::vector<std::shared_ptr<IRWHandler>> hs,
    std::function<uint64_t(uint64_t stripe_id)> stripe_id_to_parity_id)
    : hs_(std::move(hs)),
      stripe_id_to_parity_id_(std::move(stripe_id_to_parity_id)) {
  Ensures(is_power_of_2(strip_sz));
  Ensures(is_multiple_of(strip_sz, kAlignmentRequiredMin));
  Ensures(!(hs_.size() < 3));
  Ensures(std::ranges::all_of(
      hs_, [](auto const &h) { return static_cast<bool>(h); }));
  Ensures(stripe_id_to_parity_id_);

  auto cfg = mm::make_unique_aligned<struct static_cfg>(
      hardware_destructive_interference_size);
  cfg->strip_sz = strip_sz;
  cfg->stripe_data_sz = cfg->strip_sz * (hs_.size() - 1);
  cfg->stripe_sz = cfg->stripe_data_sz + cfg->strip_sz;

  static_cfg_ = mm::const_uptrwd_cast(std::move(cfg));
}

int backend::data_read(uint64_t stripe_id_from,
                       std::shared_ptr<read_query> rq) noexcept {
  Expects(!rq->buf().empty());
  Expects(rq->offset() < static_cfg_->stripe_data_sz);

  auto stripe_id{stripe_id_from};
  auto stripe_offset{rq->offset()};

  for (size_t rb{0}; rb < rq->buf().size(); ++stripe_id, stripe_offset = 0) {
    auto chunk_sz{
        std::min(static_cfg_->stripe_data_sz - stripe_offset,
                 rq->buf().size() - rb),
    };

    auto const strip_id_first{stripe_offset / static_cfg_->strip_sz};
    auto const strip_id_last{
        div_round_up(stripe_offset + chunk_sz, static_cfg_->strip_sz),
    };

    auto const strip_parity_id{stripe_id_to_strip_parity_id(stripe_id)};
    Ensures(strip_parity_id < hs_.size());

    auto const hs{
        handlers_data_view(hs_, strip_parity_id, strip_id_first,
                           strip_id_last - strip_id_first),
    };

    for (auto strip_offset{stripe_offset % static_cfg_->strip_sz};
         auto const &h : hs) {
      auto const sq_off{stripe_id * static_cfg_->strip_sz + strip_offset};
      auto const sq_sz{
          std::min(static_cfg_->strip_sz - strip_offset, chunk_sz),
      };
      auto new_rq{rq->subquery(rb, sq_sz, sq_off, rq)};
      if (auto const res{h->submit(std::move(new_rq))}) [[unlikely]] {
        return res;
      }
      rb += sq_sz;
      chunk_sz -= sq_sz;
      strip_offset = 0;
    }
    Ensures(0 == chunk_sz);
  }

  return 0;
}

int backend::parity_read(uint64_t stripe_id,
                         std::shared_ptr<read_query> rq) noexcept {
  Expects(!rq->buf().empty());
  Expects(!(rq->offset() + rq->buf().size() > static_cfg_->strip_sz));

  auto const strip_parity_id{stripe_id_to_strip_parity_id(stripe_id)};
  Ensures(strip_parity_id < hs_.size());

  return hs_[strip_parity_id]->submit(
      rq->subquery(0, std::min(static_cfg_->strip_sz, rq->buf().size()),
                   stripe_id * static_cfg_->strip_sz, rq));
}

int backend::stripe_write(uint64_t stripe_id_at,
                          std::shared_ptr<write_query> wqd,
                          std::shared_ptr<write_query> wqp) noexcept {
  Expects(wqd);
  Expects(!wqd->buf().empty());
  Expects(!(wqd->offset() + wqd->buf().size() > static_cfg_->stripe_data_sz));
  Expects(wqp);
  Expects(!wqp->buf().empty());
  Expects(!(wqp->offset() + wqp->buf().size() > static_cfg_->strip_sz));

  auto const strip_id_first{wqd->offset() / static_cfg_->strip_sz};
  auto const strip_id_last{
      div_round_up(wqd->offset() + wqd->buf().size(), static_cfg_->strip_sz),
  };

  auto const strip_parity_id{stripe_id_to_strip_parity_id(stripe_id_at)};
  Ensures(strip_parity_id < hs_.size());

  auto const hs{
      handlers_data_view(hs_, strip_parity_id, strip_id_first,
                         strip_id_last - strip_id_first),
  };

  size_t wb{0};
  for (auto strip_offset{wqd->offset() % static_cfg_->strip_sz};
       auto const &h : hs) {
    auto const sq_off{stripe_id_at * static_cfg_->strip_sz + strip_offset};
    auto const sq_sz{
        std::min(static_cfg_->strip_sz - strip_offset, wqd->buf().size() - wb),
    };
    auto new_wqd{wqd->subquery(wb, sq_sz, sq_off, wqd)};
    if (auto const res{h->submit(std::move(new_wqd))}) [[unlikely]] {
      return res;
    }
    wb += sq_sz;
    strip_offset = 0;
  }
  Ensures(!(wb < wqd->buf().size()));

  auto new_wqp{
      wqp->subquery(0, wqp->buf().size(),
                    stripe_id_at * static_cfg_->strip_sz + wqp->offset(), wqp),
  };

  if (auto const res{hs_[strip_parity_id]->submit(std::move(new_wqp))})
      [[unlikely]] {
    return res;
  }

  return 0;
}

} // namespace ublk::raidsp
