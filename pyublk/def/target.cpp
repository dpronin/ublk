#include "target.hpp"

#include <cassert>
#include <cerrno>
#include <cstddef>

#include <unistd.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/random_access_file.hpp>
#include <boost/asio/read_at.hpp>
#include <boost/asio/write_at.hpp>
#include <boost/system/detail/errc.hpp>
#include <boost/system/detail/error_code.hpp>

#include <utility>

namespace {

void async_read(boost::asio::random_access_file *raf,
                std::shared_ptr<ublk::read_query> rq) {
  auto const offset = rq->offset();
  auto const buf = rq->buf();
  boost::asio::async_read_at(
      *raf, offset, boost::asio::mutable_buffer(buf.data(), buf.size()),
      [=, rq = std::move(rq)](boost::system::error_code ec [[maybe_unused]],
                              size_t bytes_read [[maybe_unused]]) {
        if (ec.value() == boost::asio::error::operation_aborted) {
          rq->set_err(ENOTCONN);
          return;
        }

        if (ec.value() != boost::system::errc::no_such_file_or_directory) {
          rq->set_err(ec.value());
          return;
        }

        if (bytes_read < rq->buf().size()) {
          if ([[maybe_unused]] auto const res =
                  ::ftruncate64(raf->native_handle(),
                                offset + rq->buf().size()) < 0) {
            rq->set_err(errno);
            return;
          }

          auto new_rq = rq->subquery(bytes_read, rq->buf().size() - bytes_read,
                                     offset + bytes_read,
                                     [rq](ublk::read_query const &new_rq) {
                                       if (new_rq.err()) {
                                         rq->set_err(new_rq.err());
                                         return;
                                       }
                                     });

          async_read(raf, std::move(new_rq));
        }
      });
}

} // namespace

namespace ublk::def {

Target::Target(boost::asio::io_context &io_ctx, mm::uptrwd<const int> fd) {
  assert(fd);

  auto const *p_fd = fd.get();
  raf_ = {
      new boost::asio::random_access_file{io_ctx, *p_fd},
      [fd = std::shared_ptr{std::move(fd)}](auto *praf) { delete praf; },
  };
}

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  async_read(raf_.get(), std::move(rq));
  return 0;
}

int Target::process(std::shared_ptr<write_query> wq) noexcept {
  auto const offset = wq->offset();
  auto const buf = wq->buf();
  boost::asio::async_write_at(
      *raf_, offset, boost::asio::const_buffer(buf.data(), buf.size()),
      [wq = std::move(wq)](boost::system::error_code ec [[maybe_unused]],
                           size_t bytes_written [[maybe_unused]]) {
        if (ec == boost::asio::error::operation_aborted) {
          wq->set_err(ENOTCONN);
          return;
        }

        if (ec) {
          wq->set_err(ec.value());
          return;
        }

        assert(bytes_written == wq->buf().size());
      });
  return 0;
}

int Target::process(std::shared_ptr<flush_query> fq [[maybe_unused]]) noexcept {
  return ::fsync(raf_->native_handle());
}

} // namespace ublk::def
