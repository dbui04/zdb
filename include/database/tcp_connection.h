#pragma once

#include "database/byte_buffer.h"
#include "database/socket.h"
#include <cstddef>

constexpr std::size_t BUF_SIZE = 4096;

class TcpConnection {
public:
  TcpConnection(Socket);

  int fd() const;
  ssize_t send();
  ssize_t receive();
  std::span<const std::byte> in_buf() const;

private:
  Socket socket_;
  ByteBuffer input_;
  ByteBuffer output_;
};
