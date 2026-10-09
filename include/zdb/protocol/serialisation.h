#pragma once

#include "zdb/database/value.h"
#include "zdb/protocol/error.h"
#include "zdb/protocol/message.h"
#include "zdb/util/numeric.h"
#include <algorithm>
#include <bit>
#include <concepts>
#include <cstddef>
#include <expected>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <type_traits>
#include <variant>
#include <vector>

namespace zdb::protocol {

/* Value validity check */
template <std::integral T> constexpr bool is_valid(const T &v) {
  if constexpr (std::same_as<T, bool>)
    return v == true || v == false;

  return true;
}

template <std::same_as<Response::Status> T>
constexpr bool is_valid(const T &v) {
  switch (v) {
  case Response::Status::ok:
  case Response::Status::error:
    return true;
  }
  return false;
}

template <std::same_as<Request::Command> T>
constexpr bool is_valid(const T &v) {
  switch (v) {
  case Request::Command::set:
  case Request::Command::get:
  case Request::Command::del:
  case Request::Command::expire:
    return true;
  }
  return false;
}

/* Basic types */

template <typename T>
  requires std::integral<T> || std::is_enum_v<T>
std::expected<std::vector<std::byte>, SerialisationError>
serialise(const T &obj) {
  std::optional<SerialisationError> err;

  auto result = [&]() -> std::vector<std::byte> {
    if constexpr (std::same_as<T, std::size_t>) {
      auto casted = util::in_range_cast<std::uint32_t>(obj);
      if (!casted) {
        err = SerialisationError::size_type_cast_error;
        return {};
      }
      return {
          std::from_range,
          std::bit_cast<std::array<std::byte, sizeof(std::uint32_t)>>(*casted)};
    } else
      return {std::from_range,
              std::bit_cast<std::array<std::byte, sizeof(T)>>(obj)};
  }();

  if (err)
    return std::unexpected{*err};

  if constexpr (std::endian::native ==
                std::endian::big) // Default to little-endian
    std::ranges::reverse(result);

  return result;
}

template <typename T>
  requires std::integral<T> || std::is_enum_v<T>
std::expected<T, DeserialisationError>
deserialise(std::span<const std::byte, sizeof(T)> bytes) {
  T result;

  if constexpr (std::endian::native == std::endian::big)
    for (std::size_t i = 0; i < sizeof(T); ++i)
      reinterpret_cast<std::byte *>(&result)[i] = bytes[sizeof(T) - i - 1];
  else
    std::memcpy(&result, bytes.data(), sizeof(T));

  if (!is_valid(result))
    return std::unexpected{DeserialisationError::invalid_object_representation};

  return result;
}

/* std::string */

template <typename T>
  requires std::same_as<std::string, T> || std::same_as<std::string_view, T>
std::expected<std::vector<std::byte>, SerialisationError>
serialise(const T &obj) {
  return std::vector<std::byte>{std::from_range, std::as_bytes(std::span{obj})};
}

template <std::same_as<std::string> T>
std::expected<T, DeserialisationError>
deserialise(std::span<const std::byte> bytes) {
  return std::string{reinterpret_cast<const char *>(bytes.data()),
                     bytes.size()};
}

/* Database value type */

template <database::IsValueAlternative T>
std::expected<std::vector<std::byte>, SerialisationError>
serialise(const T &obj) {
  if (std::same_as<T, database::Bytes>)
    return obj;
  else {
    std::println(stderr,
                 "Serialisation: Database value type not defined, exiting...");
    std::exit(EXIT_FAILURE);
  }
}

template <std::same_as<database::Value> T>
std::expected<std::vector<std::byte>, SerialisationError>
serialise(const T &obj) {
  return std::visit(
      [](const auto &v)
          -> std::expected<std::vector<std::byte>, SerialisationError> {
        using DT = std::remove_cvref_t<decltype(v)>;
        return serialise<DT>(v);
      },
      obj);
}

template <database::IsValueAlternative T>
std::expected<T, DeserialisationError>
deserialise(std::span<const std::byte> bytes) {
  if constexpr (std::same_as<T, database::Bytes>)
    return T(std::from_range, bytes);
  else
    return std::unexpected{DeserialisationError::invalid_type};
}

/* Message type */

template <IsMessage M>
std::expected<std::vector<std::byte>, SerialisationError>
serialise(const M &obj) {
  std::vector<std::byte> result;
  std::optional<SerialisationError> err;

  auto check_and_append = [&]<typename T>(this auto &&self, const T &v) {
    if (err)
      return;

    if constexpr (std::ranges::sized_range<T>) {
      auto serialised = serialise<T>(v);

      if (!serialised) {
        err = serialised.error();
        return;
      }

      auto serialised_sz = serialise(serialised->size());

      if (!serialised_sz) {
        err = serialised_sz.error();
        return;
      }

      result.append_range(*serialised_sz);
      result.append_range(*serialised);

    } else if constexpr (std::same_as<T, database::Value>) {
      std::visit(
          [&](const auto &a) {
            using AltT = std::remove_cvref_t<decltype(a)>;
            if constexpr (std::same_as<AltT, database::Bytes>) {
              self(a);
            }
          },
          v);
    } else {
      auto serialised = serialise<T>(v);
      if (!serialised) {
        err = serialised.error();
        return;
      }
      result.append_range(*serialised);
    }
  };

  if constexpr (std::same_as<M, Request>) {
    // Request serialisation structure:
    // unsigned char cmd
    // std::uint32_t key.size()
    // key
    // std::uint32_t serialisation(value).size()
    // serialisation(value)
    // expire_at: std::int8_t{0} if absent, or std::int8_t{1} then int64_t

    check_and_append(obj.cmd);
    check_and_append(obj.key);
    check_and_append(obj.value);
    if (!obj.expire_at)
      check_and_append(std::int8_t{0});
    else {
      check_and_append(std::int8_t{1});
      check_and_append(*obj.expire_at);
    }
  } else if constexpr (std::same_as<M, Response>) {
    // Response serialisation structure:
    // unsigned char status
    // uint32_t serialisation(value).size()
    // serialisation(value)
    // uint32_t description.size()
    // description

    check_and_append(obj.status);
    check_and_append(obj.value);
    check_and_append(obj.description);
  }

  if (err)
    return std::unexpected{*err};

  return result;
}

// Deserialisation assume bytes contains the exact data (no more or less)
template <typename T>
  requires IsMessage<T>
std::expected<T, DeserialisationError>
deserialise(std::span<const std::byte> bytes) {
  T result;
  std::size_t offset = 0;
  std::optional<DeserialisationError> err;

  auto read =
      [&]<typename U>(std::size_t count =
                          sizeof(U)) -> std::expected<U, DeserialisationError> {
    if (count > bytes.size() - offset)
      return std::unexpected{DeserialisationError::inconsistent_data};

    auto data = bytes.subspan(offset).first(count);
    offset += count;

    if constexpr (std::is_integral_v<U> || std::is_enum_v<U>)
      return deserialise<U>(data.first<sizeof(U)>());
    else
      return deserialise<U>(data);
  };

  // Read the size, check for its validity, then read the obj
  auto read_sized =
      [&]<typename U>() -> std::expected<U, DeserialisationError> {
    return read.template operator()<std::uint32_t>().and_then(
        [&](std::uint32_t sz) { return read.template operator()<U>(sz); });
  };

  auto assign_val = [&]<typename U, bool sized = false>(auto &obj) {
    // Skip if an error already found
    if (err)
      return;

    auto res = sized ? read_sized.template operator()<U>()
                     : read.template operator()<U>();

    if (res) {
      obj = std::move(*res);
      if constexpr (std::is_enum_v<U>)
        if (!is_valid(obj))
          err = DeserialisationError::invalid_object_representation;
    } else
      err = res.error();
  };

  if constexpr (std::same_as<T, Request>) {

    assign_val.template operator()<Request::Command>(result.cmd);
    assign_val.template operator()<std::string, true>(result.key);
    assign_val.template operator()<database::Bytes, true>(result.value);
    std::int8_t has_expire_at{0};
    if (!err) {
      assign_val.template operator()<std::int8_t>(has_expire_at);
      switch (has_expire_at) {
      case 1:
        assign_val.template operator()<std::int64_t>(result.expire_at);
        break;
      case 0:
        break;
      default:
        err = DeserialisationError::invalid_object_representation;
      }
    }

  } else if constexpr (std::same_as<T, Response>) {
    assign_val.template operator()<Response::Status>(result.status);
    assign_val.template operator()<database::Bytes, true>(result.value);
    assign_val.template operator()<std::string, true>(result.description);
  }

  if (err)
    return std::unexpected{*err};

  if (offset != bytes.size())
    return std::unexpected{DeserialisationError::inconsistent_data};

  return std::expected<T, DeserialisationError>{result};
}

} // namespace zdb::protocol
