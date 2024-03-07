#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "raid4/target.hpp"

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

TEST(RAID4, TestReading) {
  std::array<std::shared_ptr<MockRWHandler>, 3> hs;
  std::ranges::generate(hs, [] { return std::make_shared<MockRWHandler>(); });

  ublk::raid4::Target tgt{4096 / (hs.size() - 1), {hs.begin(), hs.end()}};
  EXPECT_CALL(*hs[0], submit(An<std::shared_ptr<read_query>>())).Times(2);
  EXPECT_CALL(*hs[1], submit(An<std::shared_ptr<read_query>>())).Times(2);
  EXPECT_CALL(*hs[2], submit(An<std::shared_ptr<read_query>>())).Times(0);

  auto buf = std::vector<std::byte>{8192};
  tgt.process(read_query::create(buf, 0));
}

TEST(RAID4, TestWriting) {
  std::array<std::shared_ptr<MockRWHandler>, 3> hs;
  std::ranges::generate(hs, [] { return std::make_shared<MockRWHandler>(); });

  ublk::raid4::Target tgt{4096 / (hs.size() - 1), {hs.begin(), hs.end()}};
  EXPECT_CALL(*hs[0], submit(An<std::shared_ptr<write_query>>())).Times(2);
  EXPECT_CALL(*hs[1], submit(An<std::shared_ptr<write_query>>())).Times(2);
  EXPECT_CALL(*hs[2], submit(An<std::shared_ptr<write_query>>())).Times(2);

  auto buf = std::vector<std::byte>{8192};
  tgt.process(write_query::create(buf, 0));
}
