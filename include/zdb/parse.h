#pragma once

#include "concepts.h"
#include <cstddef>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

namespace zdb::parse {

// Serialise
template <concepts::SerialisationUnit T>
std::vector<std::byte> serialise(const T &obj) {
  std::vector<std::byte> result(sizeof(T));
  std::memcpy(result.data(), &obj, sizeof(T));
  return result;
}

template <concepts::ContiguousRange R>
  requires concepts::SerialisationUnit<std::ranges::range_value_t<R>>
std::vector<std::byte> serialise(const R &obj) {
  auto bytes = std::as_bytes(std::span{obj});
  return {bytes.begin(), bytes.end()};
}

// Deserialise
template <typename T>
  requires concepts::SerialisationUnit<T>
T deserialise(std::span<const std::byte, sizeof(T)> buf) {
  T result;
  std::memcpy(&result, buf.data(), sizeof(T));
  return result;
}

template <typename T>
  requires concepts::SerialisationUnit<T>
std::vector<T> deserialise(std::span<const std::byte> buf) {
  if (buf.size() % sizeof(T) != 0)
    throw std::invalid_argument(
        "Deserialise: buf size is not a multiple of sizeof(T)");

  std::vector<T> result(buf.size() / sizeof(T));
  std::memcpy(result.data(), buf.data(), buf.size());
  return result;
}

std::string deserialise(std::span<const std::byte>);

// template <zdb::concepts::ContiguousRange R>
//   requires std::is_trivially_copyable_v<std::ranges::range_value_t<R>> &&
//            std::constructible_from<R, std::size_t>
// R deserialise(std::span<const std::byte> buf) {
//   using T = std::ranges::range_value_t<R>;
//   if (buf.size() % sizeof(T) != 0)
//     throw std::invalid_argument(
//         "Deserialise: buf size is not a multiple of sizeof(T)");
//   R result(buf.size() / sizeof(T));
//   std::memcpy(std::ranges::data(result), buf.data(), buf.size());
//   return result;
// }
} // namespace zdb::parse
