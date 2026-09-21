#pragma once

#include "socket.h"
#include "tcp_connection.h"
#include <cstddef>
#include <optional>

class TcpListener {
public:
  TcpListener(const char *, std::optional<uint16_t>);
  int fd() const;
  TcpConnection accept();

private:
  Socket socket_ = Socket(AF_INET, SOCK_STREAM, 0);
};
