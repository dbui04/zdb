#pragma once

#include "zdb/database/database.h"
#include "zdb/net/kqueue.h"
#include "zdb/net/tcp_connection.h"
#include "zdb/net/tcp_listener.h"
#include "zdb/protocol/message.h"
#include <cstdint>
#include <unordered_map>

namespace zdb::net {

class Server {
public:
  Server(const char *ip, const std::uint16_t port);
  void run();
  void close_connection(int fd);

private:
  TcpListener listener_;
  std::unordered_map<int, TcpConnection> connections_;
  database::Database database_;
  Kqueue kq_;

  protocol::Response process_request(protocol::Request);
};

} // namespace zdb::net
