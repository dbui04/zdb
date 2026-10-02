#include "zdb/net/socket.h"
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <format>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace zdb::net {

Socket::Socket() : fd_{::socket(AF_INET, SOCK_STREAM, 0)} {
  if (fd_ == -1) {
    throw std::system_error(errno, std::generic_category(), "Socket");
  }
}

Socket::Socket(int domain, int type, int protocol)
    : fd_{::socket(domain, type, protocol)} {
  if (fd_ == -1) {
    throw std::system_error(errno, std::generic_category(), "Socket");
  }
}

Socket::Socket(int fd) : fd_{fd} {}

Socket::Socket(Socket &&other) noexcept : fd_{std::exchange(other.fd_, -1)} {}

/*
 Move assignment happens when both object already exists. Therefore if we do a
 move assignment b = std::move(a), b is already holding some fd, and it needs to
 be closed first or we would have a dangling fd.
*/
Socket &Socket::operator=(Socket &&other) noexcept {
  if (this != &other) {
    if (fd_ >= 0)
      close(fd_);
  }
  fd_ = std::exchange(other.fd_, -1);
  return *this;
}

Socket::~Socket() {
  if (fd_ >= 0)
    close(fd_);
}

int Socket::fd() const { return fd_; }

sockaddr_in make_addr(const char *ip, std::uint16_t port) {
  // See .sin_addr in https://man7.org/linux/man-pages/man7/ip.7.html
  auto addr = htonl(INADDR_LOOPBACK);
  if (ip != nullptr) {
    switch (inet_pton(AF_INET, ip, &addr)) {
    case 0:
      throw std::invalid_argument(std::format("Invalid IP address:{}", ip));
    case -1:
      throw std::system_error(errno, std::generic_category(), "Convert IP");
    }
  }
  return {
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr = {.s_addr = addr},
  };
}

void bind(int fd, sockaddr_in &address) {
  if (::bind(fd, reinterpret_cast<const sockaddr *>(&address),
             sizeof(address)) == -1) {
    throw std::system_error(errno, std::generic_category(), "Bind");
  }
}

void listen(int fd, int backlog) {
  if (::listen(fd, backlog) == -1) {
    throw std::system_error(errno, std::generic_category(), "Listen");
  }
}

void connect(int fd, sockaddr_in &address) {
  if (::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) ==
      -1) {
    throw std::system_error(errno, std::generic_category(), "Connect");
  }
}

bool set_non_blocking(int fd) {
  const int flags = fcntl(fd, F_GETFL, 0);
  return !(flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1);
}

} // namespace zdb::net
