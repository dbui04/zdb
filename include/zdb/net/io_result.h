#pragma once

#include <cstddef>

namespace zdb::net {

struct IoResult {
  enum class Status { success, block, eof, error };

  Status status;
  std::size_t bytes;
};

} // namespace zdb::net
