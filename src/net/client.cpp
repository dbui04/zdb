#include "zdb/net/client.h"
#include "zdb/net/tcp_connection.h"
#include "zdb/protocol/framing.h"
#include "zdb/protocol/message.h"
#include <cstdint>
#include <cstdlib>
#include <print>

namespace zdb::net {

Client::Client(const char *remote_ip, std::uint16_t remote_port) : conn_{{}} {
  auto address = net::make_addr(remote_ip, remote_port);
  net::connect(fd(), address);
}

int Client::fd() const { return conn_.fd(); }

void Client::send(protocol::Request &req) {
  conn_.queue_output(protocol::encode(req));
  auto result = conn_.send();
  switch (result.status) {
  case net::IoResult::Status::success:
    std::println("Client sent request successfully");
    break;
  default:
    std::println("Client: Error happened when sending request");
    std::exit(EXIT_FAILURE);
  }
}

protocol::Response Client::receive() {
  auto io_result = conn_.receive();
  switch (io_result.status) {
  case net::IoResult::Status::success: {
    auto res = protocol::try_decode<protocol::Response>(conn_.readable_input());
    if (res.has_value()) {
      return (*res).first;
    } else {
      std::println("Client: Decoding response failed");
      std::exit(EXIT_FAILURE);
    }
  }
  default:
    std::println("Client: Error happened when recv response");
    std::exit(EXIT_FAILURE);
  }
}

} // namespace zdb::net
