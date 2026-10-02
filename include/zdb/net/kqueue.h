#pragma once

#include "zdb/net/socket.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

namespace zdb::net {

using Kevent = struct kevent;

bool operator==(const Kevent &lhs, const Kevent &rhs);

// https://man.netbsd.org/kqueue.2
class Kqueue {
public:
  Kqueue();
  ~Kqueue();

  Kqueue(const Kqueue &) = delete;
  Kqueue &operator=(const Kqueue &) = delete;

  // Kqueue(Kqueue &&) noexcept;
  // Kqueue &operator=(Kqueue &&) noexcept;

  // Add event to pending_chlist_
  void add_event(std::uintptr_t ident, std::int16_t filter, std::uint16_t flags,
                 std::uint32_t fflags, std::int64_t data, void *udata);

  // Remove events with key ident from pending_chlist_. If event is
  // std::nullopt, remove all entries with key ident. Otherwise remove the
  // matching (ident, event) entry. Return false if no entries are found, true
  // if succeed.
  bool remove_events(std::uintptr_t ident, std::optional<Kevent> event);

  // Register events and return num of ready events
  std::size_t register_events();
  std::span<const Kevent> get_event_list(std::size_t);

private:
  int fd_{-1};

  std::vector<Kevent> chlist_;
  std::vector<Kevent> evlist_;

  using Key = std::pair<std::uintptr_t, std::int16_t>;

  using KeyHash = decltype([](const Key &obj) {
    auto h1 = std::hash<decltype(obj.first)>{}(obj.first);
    auto h2 = std::hash<decltype(obj.second)>{}(obj.second);
    return h1 ^ h2;
  });

  std::unordered_map<Key, Kevent, KeyHash> pending_chlist_;
};

} // namespace zdb::net
