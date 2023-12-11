#include <kungfu/yijinjing/journal/bus.h>
#include <kungfu/yijinjing/time.h>

namespace kungfu::yijinjing::journal {

void bus::on_load_page() {
  SPDLOG_TRACE("on_load_page notify all");
  notify_all();
}

void bus::notify_all() {
  produce();
  cv_.notify_all();
}

void bus::wait() {
  std::unique_lock lk(cv_mutex_);
  cv_.wait(lk, [&]() { return ready_.load(); });
}

void bus::consume() { ready_.store(false); }

void bus::produce() { ready_.store(true); }

void bus::set_trigger_frame_uid(uint64_t frame_uid) { trigger_frame_uid_ = frame_uid; }

uint64_t bus::get_trigger_frame_uid() { return trigger_frame_uid_; }

} // namespace kungfu::yijinjing::journal