#pragma once

#include <cassert>
#include <cerrno>

#include <memory>

#include <boost/sml.hpp>

#include "read_query.hpp"
#include "write_query.hpp"

#include "backend.hpp"

namespace ublk::raid1 {

namespace fsm {

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

struct ctx {
  std::unique_ptr<backend> be;
};

struct transition_table {
  auto operator()() noexcept {
    using namespace boost::sml;
    return make_transition_table(
        // online state
        *"online"_s +
            event<ev::rq> /
                [](ev::rq const &e, ctx &ctx, back::process<ev::fail> process) {
                  assert(ctx.be);
                  e.r = ctx.be->process(e.rq);
                  if (0 != e.r) [[unlikely]] {
                    process(ev::fail{});
                  }
                },
        "online"_s +
            event<ev::wq> /
                [](ev::wq const &e, ctx &ctx, back::process<ev::fail> process) {
                  assert(ctx.be);
                  e.r = ctx.be->process(e.wq);
                  if (0 != e.r) [[unlikely]] {
                    process(ev::fail{});
                  }
                },
        "online"_s + event<ev::fail> = "offline"_s,
        // offline state
        "offline"_s + event<ev::rq> / [](ev::rq const &e) { e.r = EIO; },
        "offline"_s + event<ev::wq> / [](ev::wq const &e) { e.r = EIO; });
  }
};

} // namespace fsm

} // namespace ublk::raid1
