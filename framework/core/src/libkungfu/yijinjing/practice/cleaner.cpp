
#include <kungfu/yijinjing/practice/apprentice.h>

using namespace kungfu::yijinjing::practice;
using namespace kungfu::longfist::enums;

namespace kungfu::yijinjing::practice {

cleaner::cleaner(apprentice &app) : app_(app) {}

void cleaner::on_react() {
  if (not is_cleaner_worker_required()) {
    return;
  }
  cleaning_worker_ = std::thread(&cleaner::do_clean, this);
}

void cleaner::do_clean() {
  while (true) {
    std::unique_lock lk(cv_mutex_);
    app_.get_bus()->get_cv().wait(lk, [&]() { return app_.release_page() && app_.is_live(); });
    lk.unlock();

    std::lock_guard<std::mutex> lock(quite_mutex_);
    if (m_quit_) {
      break;
    }
  }
}

bool cleaner::is_cleaner_worker_required() const { return app_.get_bus()->is_on_load_page_required(); }

cleaner::~cleaner() {
  {
    std::lock_guard<std::mutex> lock(quite_mutex_);
    m_quit_ = true;
  }

  if (cleaning_worker_.joinable()) {
    cleaning_worker_.join();
  }
}

} // namespace kungfu::yijinjing::practice