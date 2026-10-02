#include "zdb/net/kqueue.h"
#include <stdexcept>
#include <system_error>
#include <unistd.h>

namespace zdb::net {

bool operator==(const Kevent &lhs, const Kevent &rhs) {
  return lhs.ident == rhs.ident && lhs.filter == rhs.filter &&
         lhs.flags == rhs.flags && lhs.fflags == rhs.fflags &&
         lhs.data == rhs.data && lhs.udata == rhs.udata;
}

Kqueue::Kqueue() : evlist_(1) {
  fd_ = kqueue();
  if (fd_ == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "Kqueue initialisation");
  }
}

Kqueue::~Kqueue() { ::close(fd_); }

void Kqueue::add_event(std::uintptr_t ident, std::int16_t filter,
                       std::uint16_t flags, std::uint32_t fflags,
                       std::int64_t data, void *udata) {
  pending_chlist_.insert_or_assign(std::pair{ident, filter},
                                   Kevent{
                                       .ident = ident,
                                       .filter = filter,
                                       .flags = flags,
                                       .fflags = fflags,
                                       .data = data,
                                       .udata = udata,
                                   });
}

bool Kqueue::remove_events(std::uintptr_t ident, std::optional<Kevent> event) {
  if (event) {
    return pending_chlist_.erase({ident, (*event).filter});
    // auto [first, last] = pending_chlist_.equal_range({ident,
    // (*event).filter}); bool deleted = false;
    //
    // while (first != last) {
    //   if (first->second == *event) {
    //     first = pending_chlist_.erase(first);
    //     deleted = true;
    //   } else
    //     std::advance(first, 1);
    // }
    // return deleted;
  } else {
    const std::array filters{EVFILT_READ, EVFILT_WRITE};
    bool ret = false;
    for (auto i : filters) {
      while (pending_chlist_.erase({ident, i}))
        ret = 1;
    }
    return ret;
  }
}

std::size_t Kqueue::register_events() {
  for (auto &i : pending_chlist_) {
    chlist_.emplace_back();
    auto ev = i.second;
    EV_SET(&chlist_.back(), ev.ident, ev.filter, ev.flags, ev.fflags, ev.data,
           ev.udata);
  }
  pending_chlist_.clear();

  if (chlist_.size() > evlist_.size())
    evlist_.resize(std::max(chlist_.size(), evlist_.size() * 2));

  int result = kevent(fd_, chlist_.size() == 0 ? nullptr : chlist_.data(),
                      chlist_.size(), evlist_.data(), evlist_.size(), nullptr);

  chlist_.clear();

  if (result == -1)
    throw std::system_error(errno, std::generic_category(), "Kevent");

  return result;
}

std::span<const struct kevent> Kqueue::get_event_list(std::size_t sz) {
  if (sz > evlist_.size())
    throw std::invalid_argument(
        "Kqueue: Tried to get more elements than size of event list");
  return std::span{evlist_.data(), sz};
}

} // namespace zdb::net
