#include "zdb/net/server.h"
#include "zdb/net/socket.h"
#include "zdb/protocol/framing.h"
#include "zdb/protocol/message.h"
#include "zdb/util/time.h"
#include <cassert>
#include <fcntl.h>
#include <print>
#include <system_error>
#include <unistd.h>
#include <unordered_set>

namespace zdb::net {

Server::Server(const char *ip, const std::uint16_t port) : listener_{ip, port} {
  if (!set_non_blocking(listener_.fd()))
    throw std::system_error(errno, std::generic_category(), "Listener_fd");
  kq_.add_event(listener_.fd(), EVFILT_READ, EV_ADD, 0, 0, nullptr);
}

void Server::run() {
  while (true) {
    std::size_t nev = kq_.register_events();

    auto events = kq_.get_event_list(nev);

    // Track a list of fd to close after processing all events. This is to
    // prevent a connection closed before a listener accept() event, which may
    // reuse the fd and make remaining events in the loop, which belong to the
    // closed connection, wrongly apply to the new connection.
    std::unordered_set<int> to_close;

    for (auto &event : events) {

      int event_fd = static_cast<int>(event.ident);

      // No need to process event for a connection scheduled to be closed
      if (to_close.contains(event_fd))
        continue;

      if (event.flags & EV_ERROR) {
        // TODO: logging
        if (event_fd == listener_.fd()) {
          throw std::system_error(0, std::generic_category(),
                                  "Listener socket closed");
        }

        to_close.emplace(event_fd);
        continue;
      }

      if (event_fd == listener_.fd()) {
        Socket client = listener_.accept();
        if (!set_non_blocking(client.fd())) {
          std::println(stderr,
                       "Failed to set non-blocking mode for fd {}, closing.",
                       client.fd());
          continue;
        }
        // Is it possible that an existing fd is closed by an external program
        // and is still being tracked by connections_, but accept() returns the
        // same fd of the closed connection for the new one?
        const int client_fd = client.fd();
        connections_.emplace(client_fd, TcpConnection(std::move(client)));
        std::println("new conn fd: {}", client_fd);
        kq_.add_event(client_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
      } else {
        // Check if the server closed this connection before. There were a case
        // where the client shut down the connection; the server recognised and
        // removed it from the connection list. But the kqueue still returned it
        // for one more cycle.
        if (!connections_.contains(event_fd))
          continue;

        auto &conn = connections_.at(event_fd);

        if (event.filter == EVFILT_READ) {
          auto [status, byte_received] = conn.receive();

          switch (status) {
          case net::IoResult::Status::success:

            while (auto ret = protocol::try_decode<protocol::Request>(
                       conn.readable_input())) {
              auto [req, count] = std::move(*ret);
              conn.consume_input(count);
              auto res = process_request(std::move(req));
              conn.queue_output(protocol::encode(res));
              kq_.add_event(conn.fd(), EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
            }
            break;
          case net::IoResult::Status::block:
            break;
          case net::IoResult::Status::eof: {
            conn.set_read_eof();
            if (!conn.has_pending_output()) {
              to_close.emplace(event_fd);
              continue;
            }
            kq_.add_event(conn.fd(), EVFILT_READ, EV_DELETE, 0, 0, nullptr);
            break;
          }
          case net::IoResult::Status::error:
            to_close.emplace(event_fd);
            continue;
          }
        }

        else if (event.filter == EVFILT_WRITE) {
          if (event.flags & EV_EOF) {
            to_close.emplace(event_fd);
            // What if there's still an READ event in this batch?
            continue;
          }
          auto result = conn.send();
          switch (result.status) {
          case net::IoResult::Status::success:
            if (!conn.has_pending_output()) {
              if (conn.is_read_eof()) {
                to_close.emplace(event_fd);
                continue;
              }
              kq_.add_event(conn.fd(), EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
            }
            break;
          case net::IoResult::Status::error:
            to_close.emplace(event_fd);
            continue;
          default:
            break;
          }
        }
      }
    }

    for (auto i : to_close)
      close_connection(i);
  }
}

void Server::close_connection(int fd) {
  if (connections_.contains(fd)) {
    connections_.erase(fd);
    kq_.remove_events(fd, std::nullopt);
  }
}

protocol::Response Server::process_request(protocol::Request req) {
  protocol::Response res;

  switch (req.cmd) {
  case protocol::Request::Command::set:
    database_.set(std::move(req.key), std::move(req.value),
                  util::timepoint_from_unix(req.expire_at));
    res.status = protocol::Response::Status::ok;
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
    bool ret =
        database_.expire(req.key, *util::timepoint_from_unix(req.expire_at));
    if (ret)
      res.status = protocol::Response::Status::ok;
    else {
      res.status = protocol::Response::Status::error;
      res.description = std::format("EXPIRE: key {} not found!", req.key);
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
