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
  cv_.wait(lk, [&]() { return ready_; });
}

void bus::consume() {
  std::lock_guard<std::mutex> lk(cv_mutex_);
  ready_ = false;
}

void bus::produce() {
  std::lock_guard<std::mutex> lk(cv_mutex_);
  ready_ = true;
}

} // namespace kungfu::yijinjing::journal