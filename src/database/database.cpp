#include "zdb/database/database.h"
#include <print>

namespace zdb::database {

void Database::set(std::string key, Value value,
                   std::optional<util::TimePoint> expire_at) {
  Entry entry = {std::move(value), expire_at};
  std::println("Database: Added key '{}'", key);
  data_.insert_or_assign(std::move(key), std::move(entry));
}

bool Database::expire(const std::string &key, util::TimePoint expire_at) {
  if (expired(key))
    return false;

  data_.at(key).expire_at = expire_at;
  return true;
}

bool Database::contains(const std::string &key) { return data_.contains(key); }

const Value *Database::get(const std::string &key) {
  if (!contains(key) || expired(key))
    return nullptr;

  return &data_.at(key).value;
}

bool Database::del(const std::string &key) {
  if (expired(key))
    return false;

  data_.erase(key);
  return true;
}

bool Database::expired(const std::string &key) {
  if (!contains(key))
    return true;

  auto deadline = data_.at(key).expire_at;
  if (deadline && *deadline < util::Clock::now()) {
    data_.erase(key);
    return true;
  }

  return false;
}

} // namespace zdb::database
