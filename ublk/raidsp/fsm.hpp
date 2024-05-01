#pragma once

#include <cerrno>

#include <memory>

#include <boost/sml.hpp>

#include "read_query.hpp"
#include "write_query.hpp"

#include "acceptor.hpp"

namespace ublk::raidsp::fsm {

namespace ev {

struct rq {
  std::shared_ptr<read_query> rq;
  mutable int r;
};

struct wq {
  std::shared_ptr<write_query> wq;
  mutable int r;
};

struct stripecohcheck {
  uint64_t stripe_id;
  mutable bool r;
};

struct fail {};

} // namespace ev

struct transition_table {
  auto operator()() noexcept {
    using namespace boost::sml;
    return make_transition_table(
        // online state
        *"online"_s + event<ev::rq> /
                          [](ev::rq const &e, acceptor &acc,
                             back::process<ev::fail> process) {
                            e.r = acc.process(e.rq);
                            if (0 != e.r) [[unlikely]] {
                              process(ev::fail{});
                            }
                          },
        "online"_s + event<ev::wq> /
                         [](ev::wq const &e, acceptor &acc,
                            back::process<ev::fail> process) {
                           e.r = acc.process(e.wq);
                           if (0 != e.r) [[unlikely]] {
                             process(ev::fail{});
                           }
                         },
        "online"_s + event<ev::stripecohcheck> /
                         [](ev::stripecohcheck const &e, acceptor const &r) {
                           e.r = r.is_stripe_parity_coherent(e.stripe_id);
                         },
        "online"_s + event<ev::fail> = "offline"_s,
        // offline state
        "offline"_s + event<ev::rq> / [](ev::rq const &e) { e.r = EIO; },
        "offline"_s + event<ev::wq> / [](ev::wq const &e) { e.r = EIO; },
        "offline"_s + event<ev::stripecohcheck> /
                          [](ev::stripecohcheck const &e) { e.r = false; });
  }
};

} // namespace ublk::raidsp::fsm
