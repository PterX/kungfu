// SPDX-License-Identifier: Apache-2.0

#ifndef KF_TIME_H
#define KF_TIME_H

#include <memory>
#include <string>
#include <time.h>

#define KUNGFU_DATETIME_FORMAT "%F %T"
#define KUNGFU_TIMESTAMP_FORMAT "%F %T.%N"
#define KUNGFU_TRADING_DAY_FORMAT "%Y%m%d"
#define KUNGFU_HISTORY_DAY_FORMAT "%Y-%m-%d %H:%M:%S"

namespace kungfu::yijinjing {
struct time_unit {
  static constexpr int64_t MILLISECONDS_PER_SECOND = 1000;
  static constexpr int64_t MICROSECONDS_PER_MILLISECOND = 1000;
  static constexpr int64_t NANOSECONDS_PER_MICROSECOND = 1000;

  static constexpr int64_t SECONDS_PER_MINUTE = 60;
  static constexpr int64_t MINUTES_PER_HOUR = 60;
  static constexpr int64_t HOURS_PER_DAY = 24;
  static constexpr int64_t SECONDS_PER_HOUR = SECONDS_PER_MINUTE * MINUTES_PER_HOUR;

  static constexpr int64_t NANOSECONDS_PER_MILLISECOND = NANOSECONDS_PER_MICROSECOND * MICROSECONDS_PER_MILLISECOND;
  static constexpr int64_t NANOSECONDS_PER_SECOND = NANOSECONDS_PER_MILLISECOND * MILLISECONDS_PER_SECOND;
  static constexpr int64_t NANOSECONDS_PER_MINUTE = NANOSECONDS_PER_SECOND * SECONDS_PER_MINUTE;
  static constexpr int64_t NANOSECONDS_PER_HOUR = NANOSECONDS_PER_SECOND * SECONDS_PER_HOUR;
  static constexpr int64_t NANOSECONDS_PER_DAY = NANOSECONDS_PER_HOUR * HOURS_PER_DAY;

  static constexpr int64_t UTC_OFFSET = NANOSECONDS_PER_HOUR * 8;
};

struct time_point_info {
  int64_t system_clock_count;
  int64_t steady_clock_count;
};

class time {
public:
  /**
   * Get timestamp in nano seconds.
   * @return current nano time in int64_t (unix-timestamp * 1e9 + nano-part)
   */
  static int64_t now_in_nano();

  /**
   * Gets time base point of current process.
   * @return time base point which is set by the singleton method.
   */
  static time_point_info get_base();

  /**
   * Reset time base point so that multiprocess application can have synced time.
   * @param system_clock_count std::chrono::system_clock nano seconds count
   * @param steady_clock_count std::chrono::steady_clock nano seconds count
   */
  static void reset(int64_t system_clock_count, int64_t steady_clock_count);

private:
  time_point_info base_;
  time();

  static const time &get_instance();
};
} // namespace kungfu::yijinjing

#endif // YIJINJING_TIME_H