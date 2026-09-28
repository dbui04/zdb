#include "zdb/tcp_connection.h"
#include "zdb/payload.h"
#include <cassert>
#include <cerrno>
#include <stdexcept>
#include <utility>

using namespace zdb;

TcpConnection::TcpConnection(Socket socket) : socket_{std::move(socket)} {}

int TcpConnection::fd() const { return socket_.fd(); }

net::IoResult TcpConnection::send() {
  ssize_t byte_sent =
      ::send(fd(), output_.readable().data(), output_.size(), 0);
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

net::IoResult TcpConnection::receive() {
  ssize_t byte_received =
      ::recv(fd(), input_.writable(BUF_SIZE).data(), BUF_SIZE, 0);
  if (byte_received >= 0) {
    input_.commit(byte_received);
    return {net::IoResult::Status::success,
            static_cast<std::size_t>(byte_received)};
  } else {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return {net::IoResult::Status::block, 0};
    return {net::IoResult::Status::error, 0};
  }
}

std::pair<std::size_t, std::size_t> TcpConnection::parse_request() {
  // Initial num of tracked requests and buf size
  std::size_t pre_req_cnt = request_count();
  std::size_t pre_buf_sz = input_.size();

  while (input_.size() > 0) {
    payload::Request request;
    auto buf = in_buf();

    std::size_t bytes_read = request.deserialise(buf);

    // No bytes read means remaining buf constains incomplete data
    if (bytes_read == 0)
      break;

    // Commit request and buf change
    requests_.emplace(request);
    input_.consume(bytes_read);
  }
  return {request_count() - pre_req_cnt, input_.size() - pre_buf_sz};
}

std::size_t TcpConnection::request_count() { return requests_.size(); }

const payload::Request &TcpConnection::get_request() const {
  return requests_.front();
}

void TcpConnection::pop_request() {
  if (requests_.empty())
    throw std::out_of_range("Popping empty request queue");
  requests_.pop();
}

void TcpConnection::add_payload_to_send(payload::Payload &payload) {
  output_.append(payload.serialise());
}

std::span<const std::byte> TcpConnection::in_buf() const {
  return input_.readable();
}

std::span<const std::byte> TcpConnection::out_buf() const {
  return output_.readable();
}
