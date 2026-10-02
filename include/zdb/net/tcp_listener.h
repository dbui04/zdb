#pragma once

#include "zdb/net/socket.h"
#include <cstddef>
#include <cstdint>

namespace zdb::net {

class TcpListener {
public:
  TcpListener(const char *, std::uint16_t);
  int fd() const;
  Socket accept() const;

private:
  Socket socket_ = Socket(AF_INET, SOCK_STREAM, 0);
};

} // namespace zdb::net
