#include "database/tcp_connection.h"
#include <utility>

TcpConnection::TcpConnection(Socket socket) : socket_{std::move(socket)} {}

int TcpConnection::fd() const { return socket_.fd(); }

ssize_t TcpConnection::send() {
  ssize_t byte_sent =
      ::send(fd(), output_.readable().data(), output_.size(), 0);
  if (byte_sent > 0)
    output_.consume(byte_sent);
  return byte_sent;
}

ssize_t TcpConnection::receive() {
  ssize_t byte_received =
      ::recv(fd(), input_.writable(BUF_SIZE).data(), BUF_SIZE, 0);
  if (byte_received > 0)
    input_.commit(byte_received);
  return byte_received;
}

std::span<const std::byte> TcpConnection::received_buf() const {
  return input_.readable();
}
