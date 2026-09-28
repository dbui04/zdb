#pragma once

#include <span>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <vector>

namespace zdb {
class Kqueue {
public:
  Kqueue();

  // https://man.netbsd.org/kqueue.2
  void add_event(std::uintptr_t, std::uint32_t, std::uint32_t, std::uint32_t,
                 std::int64_t, void *);

  // Register events and return num of ready events
  std::size_t register_events();
  std::span<const struct kevent> get_event_list(std::size_t);

private:
  int fd_{-1};
  std::vector<struct kevent> chlist_;
  std::vector<struct kevent> evlist_;
};
} // namespace zdb
