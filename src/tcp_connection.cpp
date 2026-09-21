#include "database/tcp_connection.h"
#include <utility>

TcpConnection::TcpConnection(Socket socket) : socket_{std::move(socket)} {}

int TcpConnection::fd() const { return socket_.fd(); }
