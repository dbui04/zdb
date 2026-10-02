#include "zdb/net/client.h"
#include <netinet/in.h>
#include <print>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  zdb::net::Client client(nullptr, 2004);
  // int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  // sockaddr_in addr{.sin_family = AF_INET,
  //                  .sin_port = htons(2004),
  //                  .sin_addr = {.s_addr = htonl(INADDR_LOOPBACK)}};
  // if (connect(client_fd, reinterpret_cast<const sockaddr *>(&addr),
  //             sizeof(addr)) == -1) {
  //   // failed
  //   close(client_fd);
  //   return -1;
  // };
  // uint8_t value = 24;
  // auto buf = std::bit_cast<std::array<std::byte, sizeof(uint8_t)>>(value);
  // write(client_fd, buf.data(), buf.size());

  zdb::protocol::Request req;
  req.cmd = zdb::protocol::Request::Command::set;
  req.key = "Hello I'm key";
  client.send(req);
  switch (client.receive().status) {
  case zdb::protocol::Response::Status::ok:
    std::println("Response: Ok");
    break;
  case zdb::protocol::Response::Status::error:
    std::println("Response: Error");
    break;
  default:
    std::println("Response cannot be parsed");
  }
}
