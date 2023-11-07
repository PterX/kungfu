
#include <kungfu/yijinjing/practice/apprentice.h>

using namespace kungfu::yijinjing::practice;
using namespace kungfu::longfist::enums;

namespace kungfu::yijinjing::practice {

cleaner::cleaner(apprentice &app) : app_(app) {}

std::thread &cleaner::get_cleaning_worker() { return cleaning_worker_; }

void cleaner::on_react() {
  if (not is_cleaner_worker_required()) {
    return;
  }
  SPDLOG_INFO("using page cleaner");
  if (not cleaning_worker_.joinable()) {
    cleaning_worker_ = std::thread(&cleaner::do_clean, this);
  }
  SPDLOG_DEBUG("cleaning_worker_ thread id: {}", cleaning_worker_.get_id());
}

void cleaner::do_clean() {
  while (true) {
    app_.get_bus()->wait();
    app_.release_page();
    app_.get_bus()->consume();
    if (m_quit_) {
      break;
    }
  }
}

bool cleaner::is_cleaner_worker_required() const { return app_.get_bus()->is_on_load_page_required(); }

cleaner::~cleaner() {
  m_quit_ = true;
  app_.get_bus()->notify_all();
  if (cleaning_worker_.joinable()) {
    cleaning_worker_.join();
    SPDLOG_INFO("~cleaner cleaning_worker_ joined");
  }
}

} // namespace kungfu::yijinjing::practice