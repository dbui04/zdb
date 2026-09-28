#pragma once

#include "zdb/concepts.h"
#include <span>

namespace zdb::utils {
template <concepts::SerialisationUnit T>
std::span<const std::byte, sizeof(T)>
get_static_span(std::span<const std::byte> buf, std::size_t offset = 0) {
  if (buf.size() < sizeof(T) + offset)
    throw std::invalid_argument(
        "get_static_span: subspan size less than T's size");
  return std::span<const std::byte, sizeof(T)>{buf.subspan(offset, sizeof(T))};
}
} // namespace zdb::utils
