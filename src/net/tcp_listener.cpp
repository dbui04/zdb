#include "zdb/net/tcp_listener.h"
#include "zdb/net/socket.h"
#include <arpa/inet.h>
#include <print>
#include <system_error>

namespace zdb::net {

TcpListener::TcpListener(const char *ip, std::uint16_t port) {
  auto address = net::make_addr(ip, port);
  net::bind(fd(), address);
  net::listen(fd());

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

Socket TcpListener::accept() const {
  sockaddr_in client_addr;
  socklen_t client_addr_len{sizeof(client_addr)};
  int client_fd = ::accept(fd(), reinterpret_cast<sockaddr *>(&client_addr),
                           &client_addr_len);
  if (client_fd == -1)
    throw std::system_error(errno, std::generic_category(), "Socket");
  else
    std::println("Connected to a client");

  return Socket(client_fd);
}

} // namespace zdb::net
