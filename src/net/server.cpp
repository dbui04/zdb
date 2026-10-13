#include "zdb/net/server.h"
#include "zdb/net/io_result.h"
#include "zdb/net/socket.h"
#include "zdb/net/tcp_connection.h"
#include "zdb/protocol/error.h"
#include "zdb/protocol/framing.h"
#include "zdb/protocol/message.h"
#include "zdb/util/time.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <print>
#include <unistd.h>
#include <unordered_set>

namespace zdb::net {

Server::Server(const char *ip, const std::uint16_t port)
    : listener_{ip, port}, clients_(INIT_FD_LIST_SIZE) {
  if (!set_non_blocking(listener_.fd())) {
    std::println(stderr,
                 "Failed to set non-blocking mode for listener; exiting.");
    std::exit(EXIT_FAILURE);
  }
  kq_.add_event(listener_.fd(), EVFILT_READ, EV_ADD, 0, 0, nullptr);
  kq_.add_event(0, EVFILT_TIMER, EV_ADD, NOTE_NSECONDS, 1e9 / EXPIRY_SCAN_FREQ,
                nullptr); // Repeatedly clean up stale connections
}

void Server::run() {
  while (true) {
    auto nev = kq_.register_events();
    if (!nev)
      continue;

    auto events = kq_.get_event_list(*nev);

    // Track a list of fd to close after processing all events. This is to
    // prevent a connection closed before a listener accept() event, which may
    // reuse the fd and make remaining events in the loop, which belong to the
    // closed connection, wrongly apply to the new connection.
    std::unordered_set<int> to_close;

    for (auto &event : events) {

      int event_fd = static_cast<int>(event.ident);

      if (event.flags & EV_ERROR) {
        // TODO: logging
        if (event_fd == listener_.fd()) {
          std::println(stderr, "Kqueue reported error with listener event");
          std::exit(EXIT_FAILURE);
        }
        std::println("EV_ERROR set for client fd {}, closing...", event_fd);

        to_close.emplace(event_fd);
        continue;
      }

      if (event_fd == listener_.fd() && event.filter != EVFILT_TIMER) {
        auto [client, err] = listener_.accept();

        if (!client) {
          if (err == EMFILE || err == ENFILE) {
            // TODO: Call cleanup for expired connections here
          }
          // For other errno just retry
        } else {
          const int client_fd = (*client).fd();
          if (!set_non_blocking(client_fd) ||
              !set_sockopt(client_fd, SO_REUSEADDR) ||
              !set_sockopt(client_fd, SO_KEEPALIVE) ||
              !set_sockopt(client_fd, TCP_NODELAY, IPPROTO_TCP)) {
            std::println(stderr,
                         "Failed to set flags and options for fd {}, closing.",
                         client_fd);
            continue;
            // client would go out of scope here. Destructor auto-called and
            // close the fd.
          }
          if (static_cast<std::size_t>(client_fd) >= MAX_FD_LIST_SIZE) {
            // TODO: Should evict existing clients instead!!!!
            std::println(stderr,
                         "Client fd {}, larger than max num of clients allowed",
                         client_fd);
            std::exit(EXIT_FAILURE);
          }

          if (static_cast<std::size_t>(client_fd) >= clients_.size()) {
            clients_.resize(std::max(clients_.size() * 2,
                                     static_cast<std::size_t>(client_fd + 1)));
          }

          if (clients_[client_fd]) {
            std::println(stderr,
                         "Client fd {} created while the program still "
                         "tracks the same fd. Check the logic again!",
                         client_fd);
            std::exit(EXIT_FAILURE);
          }

          clients_[client_fd] = {.conn = TcpConnection{std::move(*client)},
                                 .last_activity = util::Clock::now()};
          kq_.add_event(client_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        }
      } else {
        // Check if the server closed this connection before. There were a case
        // where the client shut down the connection; the server recognised and
        // removed it from the connection list. But the kqueue still returned it
        // for one more cycle.

        if (event.filter == EVFILT_READ) {
          if (!clients_[event_fd] || to_close.contains(event_fd))
            continue;

          auto &conn = (*clients_[event_fd]).conn;

          auto [status, byte_received] = conn.receive();

          switch (status) {
          case net::IoResult::Status::success:
            (*clients_[event_fd]).last_activity = util::Clock::now();

            while (true) {
              auto ret = protocol::try_decode<protocol::Request>(
                  conn.readable_input());

              if (ret) {
                auto decoded = std::move(*ret);
                if (!conn.consume_input(decoded.read_bytes)) {
                  // Something went wrong with consuming the input buffer,
                  // possibly be a logic issue
                  std::println("Error consuming input buf of conn {} after "
                               "decoding request",
                               event_fd);
                  std::exit(EXIT_FAILURE);
                }
                auto res = process_request(std::move(decoded.data));

                auto encoded = protocol::encode(res);

                if (!encoded) {
                  std::println("Encoding Response failed. Check logic again. "
                               "Exiting...");
                  std::exit(EXIT_FAILURE);
                }
                conn.queue_output(*encoded);
                kq_.add_event(conn.fd(), EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
              } else {
                switch (ret.error()) {
                case FramingError::not_enough_data:
                  break;
                case FramingError::too_large_frame:
                  std::println(
                      stderr,
                      "Decoding error: Too large frame. Closing conn {}.",
                      conn.fd());
                  to_close.emplace(conn.fd());
                  break;
                case FramingError::malformed_data:
                  std::println(stderr,
                               "Decoding error: Malformed data found "
                               "in message. Closing conn {}.",
                               conn.fd());
                  to_close.emplace(conn.fd());
                  break;
                }
                break;
              }
            }
            break;

          case net::IoResult::Status::block:
            break;

          case net::IoResult::Status::eof:
            to_close.emplace(event_fd);
            continue;

          case net::IoResult::Status::error:
            to_close.emplace(event_fd);
            continue;
          }
        }

        else if (event.filter == EVFILT_WRITE) {
          if (!clients_[event_fd] || to_close.contains(event_fd))
            continue;

          auto &conn = (*clients_[event_fd]).conn;

          auto result = conn.send();
          switch (result.status) {
          case net::IoResult::Status::success:
            (*clients_[event_fd]).last_activity = util::Clock::now();
            if (!conn.has_pending_output())
              kq_.add_event(conn.fd(), EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
            break;
          case net::IoResult::Status::error:
            to_close.emplace(event_fd);
            continue;
          case net::IoResult::Status::eof:
            std::println("Client fd {} closed its reading side. Closing the "
                         "connection...",
                         event_fd);
            to_close.emplace(event_fd);
            continue;
          default:
            break;
          }
        }

        else if (event.filter == EVFILT_TIMER) {
          // Remove expired connections
          // The EXPIRY_SCAN_FREQ determines how many elements are scanned each
          // time, so that every element is scanned roughly once per sec

          std::size_t num_clients_to_scan = std::max(
              clients_.size() / EXPIRY_SCAN_FREQ, static_cast<std::size_t>(1));

          while (num_clients_to_scan--) {
            auto &client = clients_[expiry_cursor_];
            if (client) {
              auto now = util::Clock::now();
              if (now - (*client).last_activity >
                  std::chrono::seconds(CONN_TTL_SECONDS)) {
                std::println("Conn fd{} will be closed due to inactivity",
                             (*client).conn.fd());
                to_close.emplace((*client).conn.fd());
              }
            }
            expiry_cursor_ = (expiry_cursor_ + 1) % clients_.size();
          }
        }
      }
    }

    for (auto i : to_close)
      close_connection(i);
  }
}

void Server::close_connection(int fd) {
  clients_[fd] = std::nullopt;
  kq_.remove_events(fd, std::nullopt);
}

protocol::Response Server::process_request(protocol::Request req) {
  protocol::Response res;

  switch (req.cmd) {
  case protocol::Request::Command::set:
    if (req.expire_at) {
      auto expire_at = util::timepoint_from_unix(*req.expire_at);
      if (expire_at) {
        database_.set(std::move(req.key), std::move(req.value), *expire_at);
        res.status = protocol::Response::Status::ok;
      } else {
        res.status = protocol::Response::Status::error;
        switch (expire_at.error()) {
        case util::TimeError::value_overflows_clock_rep:
          res.description = std::format(
              "SET: expire_at value {} overflows clock representation!",
              *req.expire_at);
          break;
        }
      }
    } else {
      database_.set(std::move(req.key), std::move(req.value), std::nullopt);
      res.status = protocol::Response::Status::ok;
    }
    break;

  case protocol::Request::Command::get: {
    auto val = database_.get(req.key);
    if (!val) {
      res.status = protocol::Response::Status::error;
      res.description = std::format("GET: key {} not found!", req.key);
    } else {
      res.status = protocol::Response::Status::ok;
      res.value = *val;
    }
    break;
  }

  case protocol::Request::Command::expire: {
    if (!req.expire_at) {
      res.status = protocol::Response::Status::error;
      res.description =
          std::format("EXPIRE: expiration time must be specified");
      break;
    }

    auto expire_at = util::timepoint_from_unix(*req.expire_at);

    if (expire_at) {
      auto ret = database_.expire(req.key, *expire_at);
      if (ret)
        res.status = protocol::Response::Status::ok;
      else {
        res.status = protocol::Response::Status::error;
        res.description = std::format("EXPIRE: key {} not found!", req.key);
      }
    } else {
      res.status = protocol::Response::Status::error;
      res.description =
          std::format("SET: expire_at value {} overflows clock representation!",
                      *req.expire_at);
    }
    break;
  }

  case protocol::Request::Command::del: {
    bool ret = database_.del(req.key);
    if (ret)
      res.status = protocol::Response::Status::ok;
    else {
      res.status = protocol::Response::Status::error;
      res.description = std::format("DEL: key {} not found!", req.key);
    }
    break;
  }

  default:
    res.status = protocol::Response::Status::error;
    res.description = "Command is not valid";
  }

  return res;
}

} // namespace zdb::net
