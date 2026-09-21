#pragma once

#include "database/socket.h"

class TcpConnection {
public:
  TcpConnection(Socket);
  int fd() const;

private:
  Socket socket_;
};
