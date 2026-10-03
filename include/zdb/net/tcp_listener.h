#pragma once

#include "zdb/net/socket.h"
#include <cstddef>
#include <cstdint>
#include <optional>

namespace zdb::net {

class TcpListener {
public:
  TcpListener(const char *, std::uint16_t);
  int fd() const;

  // Return Socket and errno
  std::pair<std::optional<Socket>, int> accept() const;

private:
  Socket socket_ = Socket(AF_INET, SOCK_STREAM, 0);
};

} // namespace zdb::net
