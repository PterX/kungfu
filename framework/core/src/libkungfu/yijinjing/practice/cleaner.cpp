
#include <kungfu/yijinjing/practice/apprentice.h>

using namespace kungfu::yijinjing::practice;
using namespace kungfu::longfist::enums;

namespace kungfu::yijinjing::practice {

resource_manager::resource_manager(apprentice &app) : app_(app) {}

std::thread &resource_manager::get_cleaning_worker() { return cleaning_worker_; }

void resource_manager::on_react() {
  if (not is_cleaner_worker_required()) {
    return;
  }
  SPDLOG_INFO("using page resource_manager");
  if (not cleaning_worker_.joinable()) {
    cleaning_worker_ = std::thread(&resource_manager::do_management, this);
  }
  SPDLOG_DEBUG("cleaning_worker_ thread id: {}", cleaning_worker_.get_id());
}

void resource_manager::do_management() {
  while (true) {
    std::unique_lock lk(cv_mutex_);
    app_.get_bus()->get_cv().wait(lk, [&]() {
      if (m_quit_) {
        app_.release_page();
        return true;
      }

      bool flag = false;
      flag |= app_.pre_load_next_page();
      SPDLOG_DEBUG("app_.preload_next_page: {}", flag);
      flag |= app_.release_page();
      SPDLOG_DEBUG("app_.release_page: {}", flag);
      return flag and app_.is_live();
    });
    lk.unlock();

    if (m_quit_) {
      break;
    }
  }
}

bool resource_manager::is_cleaner_worker_required() const { return app_.get_bus()->is_on_load_page_required(); }

resource_manager::~resource_manager() {
  m_quit_ = true;
  app_.get_bus()->notify_all();

  if (cleaning_worker_.joinable()) {
    cleaning_worker_.join();
    SPDLOG_INFO("~resource_manager cleaning_worker_ joined");
  }
}

} // namespace kungfu::yijinjing::practice