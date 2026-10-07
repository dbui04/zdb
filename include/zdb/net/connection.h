#pragma once

#include "zdb/util/time.h"

namespace zdb::net {

struct ConnectionExpiry {
  util::TimePoint last_activity;
};

} // namespace zdb::net
