#pragma once

#include <cerrno>

#include <memory>

#include <boost/sml.hpp>

#include "read_query.hpp"
#include "write_query.hpp"

#include "backend.hpp"

namespace ublk::raid0::fsm {

namespace ev {

struct rq {
  std::shared_ptr<read_query> rq;
  mutable int r;
};

struct wq {
  std::shared_ptr<write_query> wq;
  mutable int r;
};

struct fail {};

} // namespace ev

/* clang-format off */
struct transition_table {
  auto operator()() noexcept {
    using namespace boost::sml;
    return make_transition_table(
       // online state
       *"online"_s + event<ev::rq> [ ([](ev::rq const &e, backend& r){ e.r = r.process(std::move(e.rq)); return 0 == e.r; }) ]
      , "online"_s + event<ev::rq> = "offline"_s
      , "online"_s + event<ev::wq> [ ([](ev::wq const &e, backend& r){ e.r = r.process(std::move(e.wq)); return 0 == e.r; }) ]
      , "online"_s + event<ev::wq> = "offline"_s
      , "online"_s + event<ev::fail> = "offline"_s
       // offline state
      , "offline"_s + event<ev::rq> / [](ev::rq const &e) { e.r = EIO; }
      , "offline"_s + event<ev::wq> / [](ev::wq const &e) { e.r = EIO; }
    );
  }
};
/* clang-format on */

} // namespace ublk::raid0::fsm
