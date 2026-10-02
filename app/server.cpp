#include "zdb/net/server.h"

int main() {
  const char *ip = nullptr;
  std::uint16_t port = 2004;
  zdb::net::Server server{ip, port};
  server.run();
}
