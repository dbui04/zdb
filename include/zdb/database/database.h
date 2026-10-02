#pragma once

#include "value.h"
#include "zdb/util/time.h"
#include <cstddef>
#include <optional>
#include <unordered_map>

namespace zdb::database {

// Database Entry struct
struct Entry {
  Value value;
  std::optional<util::TimePoint> expire_at;
};

// The Database object, should manage the entries and handle all db-related
// operations like get, set, del, contains
class Database {
public:
  // Pass std::string to set() since the database need ownership of the key.
  // Otherwise, string_view doesn't hold ownership so would required copying.
  // const std::string& would also require copying for a temporary argument.
  // Passing by value allows either copying or std::move(key) as a argument,
  // then can std::move again into data_
  void set(std::string key, Value value,
           std::optional<util::TimePoint> expire_at = std::nullopt);
  bool expire(const std::string &key, util::TimePoint expire_at);
  const Value *get(const std::string &key);
  bool del(const std::string &key);
  bool contains(const std::string &key);
  bool expired(const std::string &key);

private:
  std::unordered_map<std::string, Entry> data_;
};

} // namespace zdb::database
