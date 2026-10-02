#include "zdb/util/time.h"
#include <chrono>

namespace zdb::util {

// See https://en.cppreference.com/cpp/chrono/system_clock , sys_seconds as time
// point
std::optional<TimePoint>
timepoint_from_unix(std::optional<std::int64_t> unix_time) {
  if (!unix_time)
    return std::nullopt;

  const std::chrono::sys_seconds unix_timepoint{
      std::chrono::seconds(*unix_time)};

  return Clock::now() + std::chrono::duration_cast<Duration>(
                            unix_timepoint - std::chrono::system_clock::now());
}

} // namespace zdb::util
