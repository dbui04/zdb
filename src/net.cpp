#include "zdb/net.h"
#include <arpa/inet.h>
#include <format>
#include <system_error>

namespace zdb::net {
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

} // namespace zdb::net
