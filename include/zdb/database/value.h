#pragma once

#include "zdb/util/concepts.h"
#include <cstddef>
#include <variant>
#include <vector>

namespace zdb::database {

using Bytes = std::vector<std::byte>;

using Value = std::variant<Bytes>;

template <typename T>
concept IsValueAlternative = util::is_variant_alternative<T, Value>::value;
} // namespace zdb::database
