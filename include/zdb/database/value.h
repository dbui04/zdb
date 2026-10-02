#pragma once

#include <cstddef>
#include <variant>
#include <vector>

namespace zdb::database {

using Bytes = std::vector<std::byte>;

using Value = std::variant<std::monostate, Bytes>;

template <typename T>
concept IsDatabaseValue = requires(T t, Value v) { v = t; };

} // namespace zdb::database
