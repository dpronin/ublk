#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "raid0/target.hpp"

#include "read_query.hpp"
#include "rw_handler_interface.hpp"
#include "write_query.hpp"

using namespace ublk;
using namespace testing;

class MockRWHandler : public IRWHandler {
public:
  MOCK_METHOD(int, submit, (std::shared_ptr<read_query>), (noexcept));
  MOCK_METHOD(int, submit, (std::shared_ptr<write_query>), (noexcept));
};

TEST(RAID0, TestReading) {
  std::array<std::shared_ptr<MockRWHandler>, 2> hs;
  std::ranges::generate(hs, [] { return std::make_shared<MockRWHandler>(); });

  ublk::raid0::Target tgt{512, {hs.begin(), hs.end()}};
  EXPECT_CALL(*hs[0], submit(An<std::shared_ptr<read_query>>())).Times(4);
  EXPECT_CALL(*hs[1], submit(An<std::shared_ptr<read_query>>())).Times(4);

  auto buf = std::vector<std::byte>{4096};
  tgt.process(read_query::create(buf, 0));
}

TEST(RAID0, TestWriting) {
  std::array<std::shared_ptr<MockRWHandler>, 2> hs;
  std::ranges::generate(hs, [] { return std::make_shared<MockRWHandler>(); });

  ublk::raid0::Target r0{512, {hs.begin(), hs.end()}};
  EXPECT_CALL(*hs[0], submit(An<std::shared_ptr<write_query>>())).Times(4);
  EXPECT_CALL(*hs[1], submit(An<std::shared_ptr<write_query>>())).Times(4);

  auto buf = std::vector<std::byte>{4096};
  r0.process(write_query::create(buf, 0));
}
