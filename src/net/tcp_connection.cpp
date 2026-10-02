#include "zdb/net/tcp_connection.h"
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
  ssize_t byte_sent = ::send(socket_.fd(), output_.readable().data(),
                             output_.size(), MSG_NOSIGNAL);
  if (byte_sent >= 0) {
    output_.consume(byte_sent);
    return {net::IoResult::Status::success,
            static_cast<std::size_t>(byte_sent)};
  } else {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return {net::IoResult::Status::block, 0};
    return {net::IoResult::Status::error, 0};
  }
}

IoResult TcpConnection::receive() {
  ssize_t byte_received =
      ::recv(socket_.fd(), input_.writable(BUF_SIZE).data(), BUF_SIZE, 0);
  if (byte_received > 0) {
    input_.commit(byte_received);
    return {net::IoResult::Status::success,
            static_cast<std::size_t>(byte_received)};
  } else if (byte_received == 0)
    return {net::IoResult::Status::eof, 0};
  else {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return {net::IoResult::Status::block, 0};
    return {net::IoResult::Status::error, 0};
  }
}

int TcpConnection::fd() const { return socket_.fd(); }

std::span<const std::byte> TcpConnection::readable_input() const {
  return input_.readable();
}

void TcpConnection::consume_input(std::size_t count) { input_.consume(count); }

void TcpConnection::queue_output(std::vector<std::byte> bytes) {
  output_.append(bytes);
}

bool TcpConnection::has_pending_output() const { return !output_.empty(); }

void TcpConnection::set_read_eof() { read_eof_ = true; }

bool TcpConnection::is_read_eof() const { return read_eof_; }
} // namespace zdb::net
