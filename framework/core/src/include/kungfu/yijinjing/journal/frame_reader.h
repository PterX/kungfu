// SPDX-License-Identifier: Apache-2.0

#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/journal/journal.h>
namespace kungfu::yijinjing::journal {
class frame_reader : public reader {
public:
  frame_reader(const kungfu::yijinjing::io_device_ptr &io, int64_t begin, int64_t end, bool lazy)
      : reader(lazy, false), io_device_(io), begin_time_(begin), end_time_(end) {}
  frame_ptr next_frame();

private:
  kungfu::yijinjing::io_device_ptr io_device_;
  int64_t begin_time_;
  int64_t end_time_;
};
} // namespace kungfu::yijinjing::journal
