#include "zdb/server.h"
#include <sys/socket.h>

int main() {
  const char *ip = nullptr;
  std::uint16_t port = 2004;
  Server server{ip, port};
  server.run();
}
