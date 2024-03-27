#pragma once

#include <boost/interprocess/mapped_region.hpp>

namespace ublk::sys {
inline auto page_size() noexcept {
  static auto const page_size{
      boost::interprocess::mapped_region::get_page_size(),
  };
  return page_size;
}
} // namespace ublk::sys
