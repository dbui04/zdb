#pragma once

#include "zdb/database/value.h"
#include "zdb/protocol/message.h"
#include "zdb/util/concepts.h"
#include "zdb/util/span.h"
#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <format>
#include <ranges>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace zdb::protocol {

/* Basic types */

template <typename T>
  requires std::integral<T> || std::is_enum_v<T> || util::IsOptional<T>
std::array<std::byte, sizeof(T)> serialise(T obj) {
  auto result = std::bit_cast<std::array<std::byte, sizeof(T)>>(obj);

  if constexpr (std::endian::native ==
                std::endian::big) // Default to little-endian
    std::ranges::reverse(result);

  return result;
}

template <typename T>
  requires std::integral<T> || std::is_enum_v<T> || util::IsOptional<T>
T deserialise(std::span<const std::byte, sizeof(T)> bytes) {
  T result;

  if constexpr (std::endian::native == std::endian::big)
    for (std::size_t i = 0; i < sizeof(T); ++i)
      reinterpret_cast<std::byte *>(&result)[i] = bytes[sizeof(T) - i - 1];
  else
    std::memcpy(&result, bytes.data(), sizeof(T));

  return result;
}

/* std::string */

std::vector<std::byte> serialise(std::string_view obj);

std::string deserialise(std::span<const std::byte> bytes);

/* Database value type */

std::vector<std::byte> serialise(const database::Value &obj);

template <typename T>
  requires database::IsDatabaseValue<T>
T deserialise(std::span<const std::byte> bytes) {
  if constexpr (std::same_as<T, database::Bytes>)
    return T(std::from_range, bytes);
  else
    throw std::invalid_argument(std::format(
        "Deserialisation: {} is not a database value type", typeid(T).name()));
}

/* Message type */

std::vector<std::byte> serialise(const Request &obj);

std::vector<std::byte> serialise(const Response &obj);

template <typename T>
  requires IsMessage<T>
T deserialise(std::span<const std::byte> bytes) {
  T result;
  std::size_t offset = 0;

  auto read = [&]<typename U>(std::size_t count = sizeof(U)) {
    if (count > bytes.size() - offset)
      throw std::invalid_argument("Deserialisation: Not enough data to parse");

    auto data = bytes.subspan(offset).first(count);
    offset += count;

    if constexpr (database::IsDatabaseValue<U>)
      return deserialise<U>(data);
    else if constexpr (std::same_as<U, std::string>)
      return deserialise(data);
    else
      return deserialise<U>(util::to_static_span<sizeof(U)>(data));
  };

  if constexpr (std::same_as<T, Request>) {
    result.cmd = read.template operator()<Request::Command>();
    auto key_sz = read.template operator()<std::uint32_t>();
    result.key = read.template operator()<std::string>(key_sz);
    auto value_sz = read.template operator()<std::uint32_t>();
    result.value = read.template operator()<database::Bytes>(value_sz);
    auto is_expire_at = read.template operator()<std::int8_t>();
    if (is_expire_at == 1)
      result.expire_at = read.template operator()<std::int64_t>();

  } else if constexpr (std::same_as<T, Response>) {
    result.status = read.template operator()<Response::Status>();
    auto value_sz = read.template operator()<std::uint32_t>();
    result.value = read.template operator()<database::Bytes>(value_sz);
    auto description_sz = read.template operator()<std::uint32_t>();
    result.description = read.template operator()<std::string>(description_sz);
  }

  return result;
}

} // namespace zdb::protocol
