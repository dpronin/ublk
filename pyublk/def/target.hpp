#pragma once

#include <unistd.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/random_access_file.hpp>

#include "mem_types.hpp"
#include "flush_query.hpp"
#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::def {

class Target final {
public:
  explicit Target(boost::asio::io_context &io_ctx, uptrwd<const int> fd);

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;
  int process(std::shared_ptr<flush_query> fq) noexcept;

private:
  uptrwd<boost::asio::random_access_file> raf_;
};

} // namespace ublk::def
