#include "database/server.h"
#include <fcntl.h>
#include <print>
#include <system_error>

Server::Server(const char *ip, std::optional<uint16_t> port)
    : listener_{ip, port}, chlist_(1) {
  kq_ = kqueue();
  if (kq_ == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "Kqueue initialisation");
  }
  EV_SET(&chlist_[0], listener_.fd(), EVFILT_READ, EV_ADD, 0, 0, nullptr);
}

void Server::run() {
  while (true) {
    int nev = kevent(kq_, chlist_.data(), chlist_.size(), evlist_.data(),
                     evlist_.size(), nullptr);
    chlist_.clear();
    switch (nev) {
    case -1:
      throw std::system_error(errno, std::generic_category(), "Kevent");
    default:
      for (int i = 0; i < nev; ++i) {
        auto &event = evlist_[i];
        int event_fd = static_cast<int>(event.ident);

        if (event_fd == listener_.fd()) {
          int new_client_fd = listener_.accept();
          fcntl(new_client_fd, F_SETFL, O_NONBLOCK);
          connections_.emplace(new_client_fd, TcpConnection{new_client_fd});
          chlist_.emplace_back();
          EV_SET(&chlist_.back(), new_client_fd, EVFILT_READ, EV_ADD, 0, 0,
                 nullptr);
        } else {
          auto &conn = connections_.at(event_fd);
          if (event.filter == EVFILT_READ) {
            conn.receive();
          }
          if (event.filter == EVFILT_WRITE) {
            // conn.send();
          }

          if (event.flags & EV_EOF || event.flags & EV_ERROR) {
            if (event_fd == listener_.fd())
              throw std::system_error(errno, std::generic_category(),
                                      "Listener");
            std::println("{}", static_cast<int>(conn.received_buf()[0]));
            close_connection(event_fd);
            continue;
          }
        }
      }
    }
  }
}

void Server::close_connection(int fd) { connections_.erase(fd); }
