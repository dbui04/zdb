#pragma once

#include "zdb/protocol/message.h"
#include "zdb/protocol/serialisation.h"
#include "zdb/util/numeric.h"
#include <concepts>
#include <cstddef>
#include <cstring>
#include <optional>
#include <vector>

namespace zdb::protocol {

template <typename T, typename ST = std::uint32_t>
  requires IsMessage<T> && std::unsigned_integral<ST> &&
           (!std::same_as<ST, bool>)
std::vector<std::byte> encode(const T &obj) {
  // Encoding structure:
  // std::uint32_t body size in bytes
  // body (serialised)

  std::vector<std::byte> result(sizeof(ST));
  result.append_range(serialise(obj));

  auto count = util::in_range_cast<ST>(result.size() - sizeof(ST));
  auto serialised_count = serialise(count);
  std::memcpy(result.data(), serialised_count.data(), sizeof(ST));

  return result;
}

// try_decode() returns std::nullopt if can't decode; otherwise return the
// object + num of bytes processed
template <typename T, typename ST = std::uint32_t>
  requires IsMessage<T> && std::unsigned_integral<ST> &&
           (!std::same_as<ST, bool>)
std::optional<std::pair<T, std::size_t>>
try_decode(std::span<const std::byte> bytes) {

  if (bytes.size() < sizeof(ST))
    return std::nullopt;

  ST count = deserialise<ST>(bytes.first<sizeof(ST)>());

  if (bytes.size() - sizeof(ST) < count)
    return std::nullopt;

  return {
      {deserialise<T>(bytes.subspan(sizeof(ST), count)), sizeof(ST) + count}};
}

} // namespace zdb::protocol
