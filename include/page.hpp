#pragma once

#include <boost/interprocess/mapped_region.hpp>

namespace ublk {
inline auto get_page_size() {
  static const auto page_size =
      boost::interprocess::mapped_region::get_page_size();
  return page_size;
}
} // namespace ublk
