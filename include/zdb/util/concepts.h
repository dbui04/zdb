#pragma once

#include <optional>

namespace zdb::util {

template <typename T>
concept IsOptional = requires { typename T::value_type; } &&
                     std::same_as<T, std::optional<typename T::value_type>>;

} // namespace zdb::util
