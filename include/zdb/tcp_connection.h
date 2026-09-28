#pragma once

#include "zdb/byte_buffer.h"
#include "zdb/net.h"
#include "zdb/payload.h"
#include "zdb/socket.h"
#include <cstddef>
#include <queue>

constexpr std::size_t BUF_SIZE = 4096;

class TcpConnection {
public:
  TcpConnection(Socket);

  int fd() const;

  [[nodiscard]]
  zdb::net::IoResult send();

  [[nodiscard]]
  zdb::net::IoResult receive();

  std::span<const std::byte> in_buf() const;
  std::span<const std::byte> out_buf() const;
  std::pair<std::size_t, std::size_t>
  parse_request(); // return num of req processed and bytes
  std::size_t request_count();
  void pop_request();

  const zdb::payload::Request &get_request() const;

  void add_payload_to_send(zdb::payload::Payload &);

private:
  Socket socket_;
  ByteBuffer input_;
  ByteBuffer output_;
  std::queue<zdb::payload::Request> requests_;
};
