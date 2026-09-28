#include "zdb/server.h"
#include "zdb/net.h"
#include "zdb/tcp_connection.h"
#include <cassert>
#include <fcntl.h>
#include <print>
#include <unistd.h>

using namespace zdb;

Server::Server(const char *ip, std::uint16_t port) : listener_{ip, port} {
  kq_ = Kqueue();
  kq_.add_event(listener_.fd(), EVFILT_READ, EV_ADD, 0, 0, nullptr);
}

void Server::run() {
  while (true) {
    sleep(1);
    std::size_t nev = kq_.register_events();
    std::println("nev: {}", nev);

    auto events = kq_.get_event_list(nev);

    for (auto &event : events) {

      int event_fd = static_cast<int>(event.ident);

      if (event_fd == listener_.fd()) {
        int new_client_fd = listener_.accept();
        fcntl(new_client_fd, F_SETFL, O_NONBLOCK);
        connections_.emplace(new_client_fd, TcpConnection{new_client_fd});
        std::println("new conn fd: {}", new_client_fd);
        kq_.add_event(new_client_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
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

          std::size_t num_req_parsed{0};
          std::size_t byte_consumed{0};

          switch (status) {
          case net::IoResult::Status::success:
            std::tie(num_req_parsed, byte_consumed) = conn.parse_request();
            if (num_req_parsed > 0) {
              std::println("Parsed {} requests", num_req_parsed);
              process_request(conn, num_req_parsed);
              kq_.add_event(conn.fd(), EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
            }
            break;
          case net::IoResult::Status::block:
            break;
          case net::IoResult::Status::error:
            close_connection(event_fd);
            continue;
          }
        }

        if (event.filter == EVFILT_WRITE) {
          auto [status, byte_sent] = conn.send();
          switch (status) {
          case net::IoResult::Status::success:
            if (conn.out_buf().empty())
              kq_.add_event(conn.fd(), EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
            break;
          case net::IoResult::Status::error:
            close_connection(event_fd);
            continue;
          case net::IoResult::Status::block:
            break;
          }
        }

        if (event.flags & EV_EOF || event.flags & EV_ERROR) {
          close_connection(event_fd);
          continue;
        }
      }
    }
  }
}

void Server::close_connection(int fd) { connections_.erase(fd); }

void Server::process_request(TcpConnection &conn,
                             std::optional<std::size_t> to_process) {
  if (!to_process.has_value())
    to_process = conn.request_count();

  assert(to_process.value() <= conn.request_count());

  using namespace zdb::payload;

  while (to_process.value()--) {
    const Request &req = conn.get_request();
    Response res;
    switch (req.cmd_) {
    case Request::Command::get:
      if (!store_.contains(req.key_))
        res.status_ = Response::Status::key_not_found;
      else {
        res.status_ = Response::Status::ok;
        res.data_ = store_[req.key_];
      }
      break;
    case Request::Command::set:
      std::println("Got set request");
      if (store_.contains(req.key_))
        res.status_ = Response::Status::key_existed;
      else {
        res.status_ = Response::Status::ok;
        store_[req.key_] = req.value_;
      }
      break;
    case Request::Command::del:
      if (!store_.contains(req.key_))
        res.status_ = Response::Status::key_not_found;
      else {
        res.status_ = Response::Status::ok;
        store_.erase(req.key_);
      }
      break;
    }
    conn.add_payload_to_send(res);
    conn.pop_request();
  }
}
