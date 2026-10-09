#pragma once

#include <chrono>
#include <ctime>
#include <expected>

namespace zdb::util {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using Duration = Clock::duration;
using WideDuration = std::chrono::duration<__int128, Duration::period>;
using Seconds = std::chrono::seconds;

enum class TimeError { value_overflows_clock_rep };

std::expected<TimePoint, TimeError> timepoint_from_unix(std::int64_t unix_time);

} // namespace zdb::util
