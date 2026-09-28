#pragma once

#include "tcp_connection.h"
#include "zdb/payload.h"

class Client {
public:
  Client(const char *, std::uint16_t);
  int fd() const;
  void send(zdb::payload::Request &);
  zdb::payload::Response receive();

private:
  TcpConnection conn_;
};
