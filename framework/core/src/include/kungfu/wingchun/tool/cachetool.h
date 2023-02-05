#ifndef KUNGFU_TOOL_CACHE_WRITER_H
#define KUNGFU_TOOL_CACHE_WRITER_H

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/journal/common.h>
#include <kungfu/yijinjing/journal/journal.h>

namespace kungfu::wingchun::tool {

class CacheTool {
  static int64_t parse_time(const std::string &time);

public:
  CacheTool(std::string source, std::string start_time, std::string end_time, yijinjing::data::locator_ptr locator,
            bool overwrite = true);
  CacheTool(std::string source, int64_t start_time, int64_t end_time, yijinjing::data::locator_ptr locator,
            bool overwrite = true);

  virtual void run(){};

protected:
  template <typename T> void write_at(int64_t gen_time, int64_t trigger_time, const T &data) {
    valid_time(gen_time, trigger_time);
    writer_->write_at(gen_time, trigger_time, data);
  }

  void write_raw_at(int64_t gen_time, int64_t trigger_time, int32_t msg_type, uintptr_t data, uint32_t length);


  yijinjing::journal::frame_ptr next_frame() const;

  bool data_available() const;

private:
  void init(bool overwrite);
  void valid_time(int64_t gen_time, int64_t trigger_time);
  yijinjing::data::locator_ptr locator_;
  yijinjing::journal::writer_ptr writer_;
  yijinjing::journal::reader_ptr reader_;
  int64_t start_time_;
  int64_t end_time_;
  int64_t last_gen_time_{0};
  std::string source_;
};

class CacheToolWriter : public CacheTool {
public:
  CacheToolWriter(std::string source, std::string start_time, std::string end_time, yijinjing::data::locator_ptr locator) 
  : CacheTool(source, start_time, end_time, locator, true) {}

  void write_raw(int64_t time_stamp, int32_t msg_type, uintptr_t data, uint32_t length) {
    write_raw_at(time_stamp, time_stamp, msg_type, data, length);
  }
};

class CacheToolReader : public CacheTool {
public:
    CacheToolReader(std::string source, std::string start_time, std::string end_time, yijinjing::data::locator_ptr locator) 
  : CacheTool(source, start_time, end_time, locator, false) {}

  yijinjing::journal::frame_ptr next_frame() const {
    return next_frame();
  }

  bool data_available() const {
    return data_available();
  }
};

} // namespace kungfu::wingchun::tool

#endif // KUNGFU_TOOL_CACHE_WRITER_H