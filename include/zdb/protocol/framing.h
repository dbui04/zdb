#pragma once

#include "zdb/protocol/error.h"
#include "zdb/protocol/message.h"
#include "zdb/protocol/serialisation.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <print>
#include <vector>

namespace zdb::protocol {

constexpr std::uint32_t MAX_FRAME_SIZE = 134217728; // 128 MB

template <IsMessage T>
std::expected<std::vector<std::byte>, FramingError> encode(const T &obj) {
  // Encoding structure:
  // std::uint32_t body size in bytes
  // body (serialised)

  std::vector<std::byte> result;

  auto serialised = serialise<T>(obj);
  if (!serialised) {
    switch (serialised.error()) {
    case SerialisationError::size_type_cast_error:
      return std::unexpected{FramingError::too_large_frame};
      break;
    default:
      std::println("Encoding: Serialisation failed with unknown error!");
      std::exit(EXIT_FAILURE);
    }
  }

  if (serialised->size() > MAX_FRAME_SIZE)
    return std::unexpected{FramingError::too_large_frame};

  auto count = serialise(serialised->size());
  if (!count)
    return std::unexpected{FramingError::too_large_frame};

  result.append_range(*count);
  result.append_range(*serialised);

  return result;
}

template <typename T> struct DecodedData {
  T data;
  std::size_t read_bytes{0};
};

// try_decode() returns std::nullopt if can't decode; otherwise return the
// object + num of bytes processed
template <IsMessage T>
std::expected<DecodedData<T>, FramingError>
try_decode(std::span<const std::byte> bytes) {
  using ST = std::uint32_t;

  if (bytes.size() < sizeof(ST))
    return std::unexpected{FramingError::not_enough_data};

  auto count = deserialise<ST>(bytes.first<sizeof(ST)>());

  if (!count)
    return std::unexpected{FramingError::malformed_data};

  if (*count > MAX_FRAME_SIZE)
    return std::unexpected{FramingError::too_large_frame};

  if (bytes.size() - sizeof(ST) < *count)
    return std::unexpected{FramingError::not_enough_data};

  auto res = deserialise<T>(bytes.subspan(sizeof(ST), *count));
  if (!res)
    return std::unexpected{FramingError::malformed_data};

  return DecodedData<T>{*res, sizeof(ST) + *count};
}

} // namespace zdb::protocol
