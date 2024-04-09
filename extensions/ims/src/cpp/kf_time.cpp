// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <ctime>
// #include <fmt/format.h>
#include <regex>

// #include "common.h"
#include "kf_time.h"

using namespace std::chrono;

namespace kungfu::yijinjing {

#ifdef __linux__

// Make sure to use clock_gettime instead of syscall to have better performance
// https://stackoverflow.com/questions/70444744/c-linux-fastest-way-to-measure-time-faster-than-stdchrono-benchmark-incl
// https://github.com/gcc-mirror/gcc/blob/releases/gcc-11.3.0/libstdc%2B%2B-v3/src/c%2B%2B11/chrono.cc

inline int64_t get_clock_count(clockid_t clk_id) {
  timespec ts;
  clock_gettime(clk_id, &ts);
  return ts.tv_sec * time_unit::NANOSECONDS_PER_SECOND + ts.tv_nsec;
}

inline int64_t system_clock_count() { return get_clock_count(CLOCK_REALTIME); }

inline int64_t steady_clock_count() { return get_clock_count(CLOCK_MONOTONIC); }

#else

#define NOW_SINCE_EPOCH(clock) clock::now().time_since_epoch()

inline int64_t system_clock_count() { return duration_cast<nanoseconds>(NOW_SINCE_EPOCH(system_clock)).count(); }

inline int64_t steady_clock_count() { return NOW_SINCE_EPOCH(steady_clock).count(); }

#endif

int64_t time::now_in_nano() {
  auto duration = steady_clock_count() - get_instance().base_.steady_clock_count;
  return get_instance().base_.system_clock_count + duration;
}

time_point_info time::get_base() { return get_instance().base_; }

void time::reset(int64_t system_clock_count, int64_t steady_clock_count) {
  time_point_info &base = const_cast<time &>(get_instance()).base_;
  base.system_clock_count = system_clock_count;
  base.steady_clock_count = steady_clock_count;
}
/**
 * start_time_system_ sample: 1560144011373015000
 * start_time_steady_ sample: 867884767983511
 */
time::time() : base_() {
  base_.system_clock_count = system_clock_count();
  base_.steady_clock_count = steady_clock_count();
}

const time &time::get_instance() {
  static time instance = {};
  return instance;
}

} // namespace kungfu::yijinjing