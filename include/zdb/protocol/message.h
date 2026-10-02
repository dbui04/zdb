#pragma once

#include "zdb/database/value.h"
#include <cstddef>
#include <optional>
#include <string>

namespace zdb::protocol {

struct Request {
  enum class Command : unsigned char { set, get, expire, del };

  Command cmd;
  std::string key;
  database::Value value;

  // Unix time in seconds. Using std::int64_t since std::chrono::seconds
  // constructor expects signed integral type of at least 35 bits
  std::optional<std::int64_t> expire_at;
};

struct Response {
  enum class Status : unsigned char { ok, error };

  Status status;
  database::Value value;
  std::string description;
};

template <typename T>
concept IsMessage = std::same_as<T, Request> || std::same_as<T, Response>;

} // namespace zdb::protocol
