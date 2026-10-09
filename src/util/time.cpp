#include "zdb/util/time.h"
#include <chrono>
#include <expected>
#include <utility>

namespace zdb::util {

// Perform arithmetic with WideDuration which uses __int128 as rep to check for
// potential steady_clock::duration::rep overflow
std::expected<TimePoint, TimeError>
timepoint_from_unix(std::int64_t unix_time) {
  const auto system_now = std::chrono::system_clock::now();
  const auto clock_now = Clock::now();

  const auto offset = WideDuration{Seconds{unix_time}} -
                      WideDuration{system_now.time_since_epoch()};

  const auto res = WideDuration{clock_now.time_since_epoch()} + offset;

  if (!std::in_range<Duration::rep>(res.count()))
    return std::unexpected{TimeError::value_overflows_clock_rep};

  return TimePoint{Duration{res}};
}

} // namespace zdb::util
