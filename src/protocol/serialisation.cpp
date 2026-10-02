#include "zdb/protocol/serialisation.h"
#include "zdb/util/numeric.h"
#include <ranges>

namespace zdb::protocol {

std::vector<std::byte> serialise(std::string_view obj) {
  return {std::from_range, std::as_bytes(std::span{obj})};
}

std::string deserialise(std::span<const std::byte> bytes) {
  return {reinterpret_cast<const char *>(bytes.data()), bytes.size()};
}

std::vector<std::byte> serialise(const database::Value &obj) {
  return std::visit(
      [](const auto &v) -> std::vector<std::byte> {
        using T = std::remove_cvref_t<decltype(v)>;
        if constexpr (std::same_as<T, std::monostate>)
          return {};
        else
          return {std::from_range, v};
      },
      obj);
}

std::vector<std::byte> serialise(const Request &obj) {
  std::vector<std::byte> result;

  // Request serialisation structure:
  // unsigned char cmd
  // std::uint32_t key.size()
  // key
  // std::uint32_t serialisation(value).size()
  // serialisation(value)
  // expire_at: std::int8_t{0} if absent, or std::int8_t{1} then int64_t

  result.append_range(serialise(obj.cmd));
  result.append_range(
      serialise(util::in_range_cast<std::uint32_t>(obj.key.size())));
  result.append_range(std::as_bytes(std::span{obj.key}));
  std::vector<std::byte> serialised_value = serialise(obj.value);
  result.append_range(
      serialise(util::in_range_cast<std::uint32_t>(serialised_value.size())));
  result.append_range(serialised_value);
  if (!obj.expire_at)
    result.append_range(serialise(std::int8_t{0}));
  else {
    result.append_range(serialise(std::int8_t{1}));
    result.append_range(serialise(*obj.expire_at));
  }

  return result;
}

std::vector<std::byte> serialise(const Response &obj) {
  std::vector<std::byte> result;

  // Response serialisation structure:
  // unsigned char status
  // uint32_t serialisation(value).size()
  // serialisation(value)
  // uint32_t description.size()
  // description

  result.append_range(serialise(obj.status));
  std::vector<std::byte> serialised_value = serialise(obj.value);
  result.append_range(
      serialise(util::in_range_cast<std::uint32_t>(serialised_value.size())));
  result.append_range(serialised_value);
  result.append_range(
      serialise(util::in_range_cast<std::uint32_t>(obj.description.size())));
  result.append_range(std::as_bytes(std::span{obj.description}));

  return result;
}

} // namespace zdb::protocol
