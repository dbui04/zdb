#include "database/socket.h"
#include <cerrno>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <utility>

Socket::Socket(int domain, int type, int protocol)
    : fd_{socket(domain, type, protocol)} {
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
