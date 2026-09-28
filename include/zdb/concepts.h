#pragma once

#include <ranges>

namespace zdb::concepts {
template <typename T>
concept SerialisationUnit =
    std::is_trivially_copyable_v<T> && !std::ranges::range<T>;

template <typename R>
concept ContiguousRange =
    std::ranges::contiguous_range<R> && std::ranges::sized_range<R>;
} // namespace zdb::concepts
