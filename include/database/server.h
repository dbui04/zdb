#pragma once

#include "database/tcp_connection.h"
#include "tcp_listener.h"
#include <cstdint>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unordered_map>

constexpr std::size_t EVLIST_SZ = 4000;
class Server {
public:
  Server(const char *, std::optional<uint16_t>);
  void run();
  void close_connection(int);

private:
  TcpListener listener_;
  std::unordered_map<int, TcpConnection> connections_;
  int kq_{-1};
  std::vector<struct kevent> chlist_;
  std::array<struct kevent, EVLIST_SZ> evlist_;
};
