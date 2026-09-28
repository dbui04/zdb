#include "zdb/kqueue.h"
#include <stdexcept>
#include <system_error>

namespace zdb {

Kqueue::Kqueue() : evlist_(1) {
  fd_ = kqueue();
  if (fd_ == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "Kqueue initialisation");
  }
}

void Kqueue::add_event(std::uintptr_t ident, std::uint32_t filter,
                       std::uint32_t flags, std::uint32_t fflags,
                       std::int64_t data, void *udata) {
  chlist_.emplace_back();
  EV_SET(&chlist_.back(), ident, filter, flags, fflags, data, udata);
}

std::size_t Kqueue::register_events() {
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

} // namespace zdb
