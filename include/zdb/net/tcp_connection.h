#pragma once

#include "zdb/net/io_result.h"
#include "zdb/net/socket.h"
#include "zdb/util/byte_buffer.h"
#include <cstddef>

constexpr std::size_t BUF_SIZE = 4096;

namespace zdb::net {

class TcpConnection {
public:
  explicit TcpConnection(Socket);

  [[nodiscard]]
  IoResult send();

  [[nodiscard]]
  IoResult receive();

  int fd() const;

  std::span<const std::byte> readable_input() const;
  void consume_input(std::size_t count);
  void queue_output(std::vector<std::byte> bytes);
  bool has_pending_output() const;

  void set_read_eof();
  bool is_read_eof() const;

private:
  Socket socket_;
  util::ByteBuffer input_;
  util::ByteBuffer output_;
  bool read_eof_ = false;
};

} // namespace zdb::net
