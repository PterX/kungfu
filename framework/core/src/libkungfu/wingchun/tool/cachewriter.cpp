#include <kungfu/wingchun/common.h>
#include <kungfu/yijinjing/journal/assemble.h>
#include <kungfu/wingchun/tool/cachewriter.h>

using kungfu::yijinjing::time;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::data;
namespace fs = std::filesystem;
namespace kungfu::wingchun::tool {

int64_t CacheWriter::parse_time(const std::string &time_string) {
  int64_t time_stamp =
      time::strptime(time_string, {KUNGFU_DATETIME_FORMAT, KUNGFU_TRADING_DAY_FORMAT, KUNGFU_TIMESTAMP_FORMAT, KUNGFU_HISTORY_DAY_FORMAT});
  if (time_stamp < 0) {
    throw wingchun_error(fmt::format("invalid time format: {}", time_string));
  }
  return time_stamp;
}

CacheWriter::CacheWriter(std::string source, std::string start_time, std::string end_time, locator_ptr locator)
    : source_(std::move(source)), start_time_(parse_time(start_time)), end_time_(parse_time(end_time)), locator_(std::move(locator)) {
  init_writer();
}

CacheWriter::CacheWriter(std::string source, int64_t start_time, int64_t end_time, locator_ptr locator)
    : source_(std::move(source)), start_time_(start_time), end_time_(end_time), locator_(std::move(locator)) {
  init_writer();
}

void CacheWriter::init_writer() {
  if (end_time_ < start_time_ or start_time_ < 0) {
    throw wingchun_error(
        fmt::format("invalid time interval: start_time={} later than end_time={}", time::strftime(start_time_), time::strftime(end_time_)));
  }
  uint32_t cache_uid = hash_backtest_cache(start_time_, end_time_);
  auto cache_location = location::make_shared(mode::BACKTEST, category::MD, source_, fmt::format("{:08x}", cache_uid), locator_);
  auto publisher = std::make_shared<yijinjing::journal::noop_publisher>();
  std::string cache_dir = locator_->layout_dir(cache_location, layout::JOURNAL);
  fs::remove_all(cache_dir);
  writer_ = std::make_shared<yijinjing::journal::writer>(cache_location , location::PUBLIC, true, publisher);
}

void CacheWriter::valid_time(int64_t gen_time, int64_t trigger_time) {
  if (gen_time < trigger_time or trigger_time < start_time_ or gen_time > end_time_) {
    throw wingchun_error(fmt::format("invalid time: gen_time={}, trigger_time={}", gen_time, trigger_time));
  }
  if (trigger_time < last_gen_time_) {
    throw wingchun_error(fmt::format("invalid time: trigger_time={} < last_gen_time_={}", trigger_time, last_gen_time_));
  }
  last_gen_time_ = gen_time;
  }
} // namespace kungfu::wingchun::tool