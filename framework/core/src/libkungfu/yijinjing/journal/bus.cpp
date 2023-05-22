#include <kungfu/yijinjing/journal/bus.h>
#include <kungfu/yijinjing/time.h>

namespace kungfu::yijinjing::journal {

void bus::on_load_page() {
  SPDLOG_TRACE("on_load_page notify all");
  notify_all();
}

void bus::notify_all() { cv_.notify_all(); }

} // namespace kungfu::yijinjing::journal