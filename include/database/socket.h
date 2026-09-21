#pragma once

#include <sys/socket.h>

// Prohibit copy, move only due to possible close() of same fd multiple times
class Socket {
public:
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
