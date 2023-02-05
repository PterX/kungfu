#include <kungfu/wingchun/common.h>
#include <kungfu/wingchun/tool/cachetool.h>
#include <kungfu/yijinjing/journal/assemble.h>

using kungfu::yijinjing::time;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;
namespace fs = std::filesystem;
namespace kungfu::wingchun::tool {

int64_t CacheTool::parse_time(const std::string &time_string) {
  int64_t time_stamp = time::strptime(time_string, {KUNGFU_DATETIME_FORMAT, KUNGFU_TRADING_DAY_FORMAT,
                                                    KUNGFU_TIMESTAMP_FORMAT, KUNGFU_HISTORY_DAY_FORMAT});
  if (time_stamp < 0) {
    throw wingchun_error(fmt::format("invalid time format: {}", time_string));
  }
  return time_stamp;
}

CacheTool::CacheTool(std::string source, std::string start_time, std::string end_time, locator_ptr locator, bool overwrite)
    : source_(std::move(source)), start_time_(parse_time(start_time)), end_time_(parse_time(end_time)),
      locator_(std::move(locator)) {
  init(overwrite);
}

CacheTool::CacheTool(std::string source, int64_t start_time, int64_t end_time, locator_ptr locator, bool overwrite)
    : source_(std::move(source)), start_time_(start_time), end_time_(end_time), locator_(std::move(locator)) {
  init(overwrite);
}

void CacheTool::write_raw_at(int64_t gen_time, int64_t trigger_time,  int32_t msg_type, uintptr_t data, uint32_t length) {
  valid_time(gen_time, trigger_time);
  auto frame = writer_->open_frame(trigger_time, msg_type, length);
  memcpy(const_cast<void *>(frame->data_address()), reinterpret_cast<void *>(data), length);
  writer_->close_frame(length, gen_time);
}

frame_ptr CacheTool::next_frame() const { 
  if (reader_->data_available()) {
    reader_->next();
    return reader_->current_frame();
  }
  return {};
}

bool CacheTool::data_available() const { return reader_->data_available(); }

void CacheTool::init(bool overwrite) {
  if (end_time_ < start_time_ or start_time_ < 0) {
    throw wingchun_error(fmt::format("invalid time interval: start_time={} later than end_time={}",
                                     time::strftime(start_time_), time::strftime(end_time_)));
  }
  uint32_t cache_uid = hash_backtest_cache(start_time_, end_time_);
  auto cache_location =
      location::make_shared(mode::BACKTEST, category::MD, source_, fmt::format("{:08x}", cache_uid), locator_);
  auto publisher = std::make_shared<yijinjing::journal::noop_publisher>();
  if (overwrite) {
    std::string cache_dir = locator_->layout_dir(cache_location, layout::JOURNAL);
    fs::remove_all(cache_dir);
  }
  writer_ = std::make_shared<yijinjing::journal::writer>(cache_location, location::PUBLIC, true, publisher);
  reader_ = std::make_shared<yijinjing::journal::reader>(true);
  reader_->join(cache_location, location::PUBLIC, 0);
}

void CacheTool::valid_time(int64_t gen_time, int64_t trigger_time) {
  if (gen_time < trigger_time or trigger_time < start_time_ or gen_time > end_time_) {
    throw wingchun_error(fmt::format("invalid time: gen_time={}, trigger_time={}", gen_time, trigger_time));
  }
  if (trigger_time < last_gen_time_) {
    throw wingchun_error(
        fmt::format("invalid time: trigger_time={} < last_gen_time_={}", trigger_time, last_gen_time_));
  }
  last_gen_time_ = gen_time;
}} // namespace kungfu::wingchun::tool