#ifndef KUNGFU_TOOL_CACHE_WRITER_H
#define KUNGFU_TOOL_CACHE_WRITER_H

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/journal/journal.h>

namespace kungfu::wingchun::tool {

class CacheTool {
  static int64_t parse_time(const std::string &time);

public:
  CacheTool(longfist::types::category category, std::string source, std::string start_time, std::string end_time, yijinjing::data::locator_ptr locator,
            bool overwrite = true);
  CacheTool(longfist::types::category category, std::string source, int64_t start_time, int64_t end_time, yijinjing::data::locator_ptr locator,
            bool overwrite = true);

  int64_t get_start_time() const { return begin_time_;}

  int64_t get_end_time() const { return end_time_;}


 virtual void run(){};

protected:
  template <typename T> void write_at(int64_t gen_time, int64_t trigger_time, uint32_t dest_id, const T &data) {
    valid_time(gen_time, trigger_time);
    if (writers_.find(dest_id) == writers_.end()) {
      writers_[dest_id] = std::make_shared<yijinjing::journal::writer>(cache_location_, dest_id, true, publisher_);
    }
    writers_.at(dest_id)->write_at(gen_time, trigger_time, data);
  }

  void write_raw_at(int64_t gen_time, int64_t trigger_time, uint32_t dest_id, int32_t msg_type, uintptr_t data, uint32_t length);


  yijinjing::journal::frame_ptr next_frame() const;

  bool data_available() const;

  int64_t get_last_read_gen_time() const { return last_read_gen_time_;}

  void join(uint32_t dest_id, const int64_t from_time);

private:
  void init(bool overwrite);
  void valid_time(int64_t gen_time, int64_t trigger_time);
  longfist::types::category category_;
  std::string source_;
  yijinjing::data::locator_ptr locator_;
  yijinjing::data::location_ptr cache_location_;
  yijinjing::publisher_ptr publisher_;
  std::unordered_map<uint32_t, yijinjing::journal::writer_ptr> writers_ = {};
  yijinjing::journal::reader_ptr reader_;
  int64_t begin_time_;
  int64_t end_time_;
  int64_t last_gen_time_;
  mutable int64_t last_read_gen_time_;
};

class CacheToolWriter : public CacheTool {
public:
  CacheToolWriter(longfist::types::category category, std::string source, std::string start_time, std::string end_time, yijinjing::data::locator_ptr locator) 
  : CacheTool(category, source, start_time, end_time, locator, true) {}

  void write_raw(int64_t time_stamp, int32_t msg_type, uint32_t dest_id, uintptr_t data, uint32_t length) {
    write_raw_at(time_stamp, time_stamp, dest_id, msg_type, data, length);
  }
};

class CacheToolReader : public CacheTool {
public:
    CacheToolReader(longfist::types::category category, std::string source, std::string start_time, std::string end_time, yijinjing::data::locator_ptr locator) 
  : CacheTool(category, source, start_time, end_time, locator, false) {}

  yijinjing::journal::frame_ptr next_frame() const {
    return CacheTool::next_frame();
  }

  bool data_available() const {
    return CacheTool::data_available();
  }

  void join(uint32_t dest_id) {
    return CacheTool::join(dest_id, get_last_read_gen_time());
  }
};

} // namespace kungfu::wingchun::tool

#endif // KUNGFU_TOOL_CACHE_WRITER_H