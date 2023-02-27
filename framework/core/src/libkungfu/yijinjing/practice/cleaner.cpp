
#include <kungfu/yijinjing/practice/apprentice.h>

using namespace kungfu::yijinjing::practice;
using namespace kungfu::longfist::enums;

namespace kungfu::yijinjing::practice {

cleaner::cleaner(apprentice &app) : app_(app) {
  if (app_.is_cleaner_required()) {
    cleaning_thread_ = std::thread(&cleaner::do_clean, this);
    cleaning_thread_.detach();
  }
}

void cleaner::do_clean() {
  while (true) {
    app_.release_page();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  }
}

} // namespace kungfu::yijinjing::practice