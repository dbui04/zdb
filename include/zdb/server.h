#pragma once

#include "kqueue.h"
#include "tcp_listener.h"
#include "zdb/tcp_connection.h"
#include <cstdint>
#include <optional>
#include <unordered_map>

class Server {
public:
  Server(const char *, std::uint16_t);
  void run();
  void close_connection(int);

private:
  TcpListener listener_;
  std::unordered_map<int, TcpConnection> connections_;
  std::unordered_map<std::string, std::vector<std::byte>> store_;
  zdb::Kqueue kq_;

  void process_request(TcpConnection &, std::optional<std::size_t>);
};
