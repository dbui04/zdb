#include "database/tcp_connection.h"
#include "database/tcp_listener.h"
#include <sys/socket.h>

int main() {
  const char *ip = nullptr;
  std::optional<uint16_t> port = 2004;
  TcpListener server = TcpListener(ip, port);

  while (true) {
    TcpConnection conn = server.accept();
  }
}
