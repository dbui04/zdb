#pragma once

#include "zdb/database/database.h"
#include "zdb/net/kqueue.h"
#include "zdb/net/tcp_connection.h"
#include "zdb/net/tcp_listener.h"
#include "zdb/protocol/message.h"
#include "zdb/util/time.h"
#include <cstdint>

namespace zdb::net {

constexpr std::size_t INIT_FD_LIST_SIZE = 256;
constexpr std::size_t MAX_FD_LIST_SIZE = 4096;
constexpr std::int64_t CONN_TTL_SECONDS = 300;
constexpr std::int16_t EXPIRY_SCAN_FREQ = 20; // int16_t to ensure <= 1e9 ns

static_assert(EXPIRY_SCAN_FREQ != 0);

struct ClientRef {
  TcpConnection conn;
  util::TimePoint last_activity;
};

class Server {
public:
  Server(const char *ip, const std::uint16_t port);
  void run();
  void close_connection(int fd);

private:
  TcpListener listener_;
  std::vector<std::optional<ClientRef>> clients_;

  // Increment through clients_ cyclically from time to time. Each time scan
  // a certain number of clients and evict expired ones. Set the scanning
  // frequency  with EXPIRY_SCAN_FREQ.
  std::size_t expiry_cursor_{0};

  database::Database database_;
  Kqueue kq_;

  protocol::Response process_request(protocol::Request);

  void kill_unresponsive_conns();
};

} // namespace zdb::net
