#pragma once

#include <cstddef>
#include <cstdint>
#include <netinet/in.h>

namespace zdb::net {
struct IoResult {
  enum class Status { success, block, error };

  Status status;
  std::size_t bytes;
};

sockaddr_in make_addr(const char *, std::uint16_t);
void bind(int, sockaddr_in &);
void listen(int, int backlog = SOMAXCONN);
void connect(int fd, sockaddr_in &);

} // namespace zdb::net
