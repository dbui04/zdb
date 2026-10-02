#pragma once

#include <span>
#include <stdexcept>

namespace zdb::util {

template <std::size_t N, typename T>
constexpr auto to_static_span(std::span<T> span) {
  if (span.size() != N)
    throw std::invalid_argument("Incorrect span size");

  return std::span<T, N>{span.data(), N};
}

} // namespace zdb::util
