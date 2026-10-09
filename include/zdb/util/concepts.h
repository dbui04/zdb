#pragma once

#include <optional>
#include <type_traits>
#include <variant>

namespace zdb::util {

template <typename T>
concept IsOptional = requires { typename T::value_type; } &&
                     std::same_as<T, std::optional<typename T::value_type>>;

template <typename T, typename Variant>
struct is_variant_alternative : std::false_type {};

template <typename T, typename... Ts>
struct is_variant_alternative<T, std::variant<Ts...>>
    : std::bool_constant<(std::same_as<T, Ts> || ...)> {};

} // namespace zdb::util
