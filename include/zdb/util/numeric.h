#pragma once

#include <concepts>
#include <stdexcept>
#include <utility>

namespace zdb::util {

template <std::integral To, std::integral From>
constexpr To in_range_cast(From v) {
  if (!std::in_range<To>(v))
    throw std::out_of_range("in_range_cast(): conversion out of range");

  return static_cast<To>(v);
}

} // namespace zdb::util
