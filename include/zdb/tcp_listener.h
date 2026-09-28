#pragma once

#include "socket.h"
#include <cstddef>
#include <cstdint>

class TcpListener {
public:
  TcpListener(const char *, std::uint16_t);
  int fd() const;
  int accept();

private:
  Socket socket_ = Socket(AF_INET, SOCK_STREAM, 0);
};
