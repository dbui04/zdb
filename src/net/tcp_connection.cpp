#include "zdb/net/tcp_connection.h"
#include "zdb/net/io_result.h"
#include <cassert>
#include <cerrno>
#include <utility>

namespace zdb::net {

TcpConnection::TcpConnection(Socket socket) : socket_{std::move(socket)} {}

IoResult TcpConnection::send() {
  // Set MSG_NOSIGNAL flag to ignore SIGPIPE in case the socket was closed by
  // remote peer, which could terminate the program. Ignore the signal and
  // handle EPIPE instead.
  // See https://man7.org/linux/man-pages/man2/sendmsg.2.html
  auto byte_sent = ::send(socket_.fd(), output_.readable().data(),
                          output_.size(), MSG_NOSIGNAL);
  if (byte_sent >= 0) {
    if (!output_.consume(byte_sent))
      return {net::IoResult::Status::error,
              static_cast<std::size_t>(byte_sent)};
    return {net::IoResult::Status::success,
            static_cast<std::size_t>(byte_sent)};
  } else {
    return (errno == EAGAIN || errno == EWOULDBLOCK)
               ? IoResult{net::IoResult::Status::block, 0}
               : IoResult{net::IoResult::Status::error, 0};
  }
}

IoResult TcpConnection::receive() {
  auto byte_received =
      ::recv(socket_.fd(), input_.writable(BUF_SIZE).data(), BUF_SIZE, 0);
  if (byte_received > 0) {
    if (!input_.commit(byte_received)) {
      return {net::IoResult::Status::error,
              static_cast<std::size_t>(byte_received)};
    }
    return {net::IoResult::Status::success,
            static_cast<std::size_t>(byte_received)};
  } else if (byte_received == 0) {
    return {net::IoResult::Status::eof, 0};
  } else {
    return (errno == EAGAIN || errno == EWOULDBLOCK)
               ? IoResult{net::IoResult::Status::block, 0}
               : IoResult{net::IoResult::Status::error, 0};
  }
}

int TcpConnection::fd() const { return socket_.fd(); }

std::span<const std::byte> TcpConnection::readable_input() const {
  return input_.readable();
}

bool TcpConnection::consume_input(std::size_t count) {
  return input_.consume(count);
}

void TcpConnection::queue_output(std::vector<std::byte> bytes) {
  output_.append(bytes);
}

bool TcpConnection::has_pending_output() const { return !output_.empty(); }

} // namespace zdb::net
