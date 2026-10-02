#pragma once

#include "zdb/net/tcp_connection.h"
#include "zdb/protocol/message.h"

namespace zdb::net {

class Client {
public:
  Client(const char *, std::uint16_t);
  int fd() const;
  void send(protocol::Request &);
  protocol::Response receive();

private:
  TcpConnection conn_;
};

} // namespace zdb::net
