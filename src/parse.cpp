#include "zdb/parse.h"

namespace zdb::parse {
std::string deserialise(std::span<const std::byte> buf) {
  std::string result(buf.size(), '\0');
  std::memcpy(result.data(), buf.data(), buf.size());
  return result;
}
} // namespace zdb::parse
