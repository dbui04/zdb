#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>

namespace zdb::net {

// Prohibit copy, move only due to possible close() of same fd multiple times
class Socket {
public:
  Socket();
  Socket(int);
  Socket(int, int, int);
  Socket(const Socket &) = delete;
  Socket(Socket &&) noexcept;

  Socket &operator=(const Socket &) = delete;
  Socket &operator=(Socket &&) noexcept;

  ~Socket();

  int fd() const;

private:
  int fd_ = -1;
};

sockaddr_in make_addr(const char *ip, std::uint16_t port);
void bind(int fd, sockaddr_in &);
void listen(int, int backlog = SOMAXCONN);
void connect(int fd, sockaddr_in &);
bool set_non_blocking(int fd);

} // namespace zdb::net
