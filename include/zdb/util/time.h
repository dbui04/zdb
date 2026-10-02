#pragma once

#include <chrono>
#include <ctime>
#include <optional>

namespace zdb::util {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;

std::optional<TimePoint>
timepoint_from_unix(std::optional<std::int64_t> unix_time);

} // namespace zdb::util
