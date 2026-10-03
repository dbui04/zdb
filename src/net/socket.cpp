#include "zdb/net/socket.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <format>
#include <print>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace zdb::net {

Socket::Socket() : fd_{::socket(AF_INET, SOCK_STREAM, 0)} {}

Socket::Socket(int domain, int type, int protocol)
    : fd_{::socket(domain, type, protocol)} {}

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

sockaddr_in make_addr(const char *ip, std::uint16_t port) noexcept {
  // See .sin_addr in https://man7.org/linux/man-pages/man7/ip.7.html
  auto addr = htonl(INADDR_LOOPBACK);
  if (ip != nullptr) {
    switch (inet_pton(AF_INET, ip, &addr)) {
    case 0:
      std::println(stderr, "Invalid IP address: {}", ip);
      std::exit(EXIT_FAILURE);
    case -1:
      std::println(stderr, "Invalid AF family");
      std::exit(EXIT_FAILURE);
    }
  }
  return {
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr = {.s_addr = addr},
  };
}

void bind(int fd, sockaddr_in &address) noexcept {
  if (::bind(fd, reinterpret_cast<const sockaddr *>(&address),
             sizeof(address)) == -1) {
    std::println("Bind failed, errno: {}", errno);
    std::exit(EXIT_FAILURE);
  }
}

void listen(int fd, int backlog) noexcept {
  if (::listen(fd, backlog) == -1) {
    std::println("Listen failed, errno: {}", errno);
    std::exit(EXIT_FAILURE);
  }
}

void connect(int fd, sockaddr_in &address) {
  if (::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) ==
      -1) {
    std::println("Connect failed, errno: {}", errno);
    std::exit(EXIT_FAILURE);
  }
}

bool set_non_blocking(int fd) {
  const int flags = fcntl(fd, F_GETFL, 0);
  return !(flags == -1 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1);
}

} // namespace zdb::net
