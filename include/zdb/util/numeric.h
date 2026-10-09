#pragma once

#include <concepts>
#include <expected>
#include <utility>

namespace zdb::util {

enum class NumericError { out_of_range };

template <std::integral To, std::integral From>
constexpr std::expected<To, NumericError> in_range_cast(From v) {
  if (!std::in_range<To>(v))
    return std::unexpected{NumericError::out_of_range};

  return static_cast<To>(v);
}

} // namespace zdb::util
