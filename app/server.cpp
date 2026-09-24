#include "database/server.h"
#include <print>
#include <sys/socket.h>

int main() {
  const char *ip = nullptr;
  std::optional<uint16_t> port = 2004;
  // TcpListener server = TcpListener(ip, port);
  //
  // while (true) {
  //   TcpConnection conn = server.accept();
  //   // std::array<std::byte, sizeof(int)> buf;
  //   conn.receive();
  //   std::println("{}", static_cast<int>(conn.received_buf()[0]));
  // }
  Server server{ip, port};
  server.run();
}
