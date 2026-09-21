#include "database/tcp_listener.h"
#include <arpa/inet.h>
#include <format>
#include <netinet/in.h>
#include <print>
#include <stdexcept>
#include <system_error>

TcpListener::TcpListener(const char *ip, std::optional<uint16_t> port) {
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
  sockaddr_in address{
      .sin_family = AF_INET,
      .sin_port = htons(port.value_or(0)),
      .sin_addr = {.s_addr = addr},
  };

  if (bind(fd(), reinterpret_cast<const sockaddr *>(&address),
           sizeof(address)) == -1) {
    throw std::system_error(errno, std::generic_category(), "Bind");
  }

  if (listen(fd(), SOMAXCONN) == -1) {
    throw std::system_error(errno, std::generic_category(), "Listen");
  }

  socklen_t address_len = sizeof(address);
  if (getsockname(fd(), reinterpret_cast<sockaddr *>(&address), &address_len) ==
      -1) {
    throw std::system_error(errno, std::generic_category(), "getsockname");
  }

  char print_ip[INET_ADDRSTRLEN];
  if (inet_ntop(address.sin_family, &address.sin_addr, print_ip,
                sizeof(print_ip)) == nullptr) {
    throw std::system_error(errno, std::generic_category(), "inet_ntop");
  }
  uint16_t print_port = ntohs(address.sin_port);
  std::println("Server is listening at {}:{}", print_ip, print_port);
}

int TcpListener::fd() const { return socket_.fd(); }

TcpConnection TcpListener::accept() {
  sockaddr_in client_addr;
  socklen_t client_addr_len{sizeof(client_addr)};
  int client_fd = ::accept(fd(), reinterpret_cast<sockaddr *>(&client_addr),
                           &client_addr_len);
  if (client_fd == -1)
    throw std::system_error(errno, std::generic_category(), "Accept");

  std::println("Connected to a client");
  return TcpConnection(Socket(client_fd));
}
