#ifndef KUNGFU_TOOL_CACHE_WRITER_H
#define KUNGFU_TOOL_CACHE_WRITER_H

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/journal/common.h>
#include <kungfu/yijinjing/journal/journal.h>

namespace kungfu::wingchun::tool {

class CacheWriter {
  static int64_t parse_time(const std::string &time);

public:
  CacheWriter(std::string source, std::string start_time, std::string end_time, yijinjing::data::locator_ptr locator);
  CacheWriter(std::string source, int64_t start_time, int64_t end_time, yijinjing::data::locator_ptr locator);

  // virtual void run() = 0;

// protected:
  template <typename T>
  void write_at(int64_t gen_time, int64_t trigger_time, const T &data) {
    valid_time(gen_time, trigger_time);
    writer_->write_at(gen_time, trigger_time, data);
  }

private:
  void valid_time(int64_t gen_time, int64_t trigger_time);
  void init_writer();
  kungfu::yijinjing::journal::writer_ptr writer_;
  kungfu::yijinjing::data::locator_ptr locator_;
  int64_t start_time_;
  int64_t end_time_;
  int64_t last_gen_time_{0};
  std::string source_;
};

} // namespace kungfu::yijinjing::tool

#endif // KUNGFU_TOOL_CACHE_WRITER_H