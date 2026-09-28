#include "zdb/client.h"
#include "zdb/net.h"
#include "zdb/tcp_connection.h"
#include <cstdint>
#include <print>
#include <stdexcept>

using namespace zdb;

Client::Client(const char *remote_ip, std::uint16_t remote_port) : conn_{{}} {
  auto address = net::make_addr(remote_ip, remote_port);
  net::connect(fd(), address);
}

int Client::fd() const { return conn_.fd(); }

void Client::send(payload::Request &req) {
  conn_.add_payload_to_send(req);
  auto result = conn_.send();
  switch (result.status) {
  case net::IoResult::Status::success:
    std::println("Client sent request successfully");
    break;
  default:
    throw std::invalid_argument("Error happened when sending request - client");
  }
}
