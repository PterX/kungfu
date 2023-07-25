#ifndef KUNGFU_BUS_H
#define KUNGFU_BUS_H

#include <condition_variable>
#include <kungfu/common.h>

namespace kungfu::yijinjing::journal {

class bus {
public:
  bus(const bool on_load_page_required) : on_load_page_required_(on_load_page_required){};

  virtual ~bus() = default;

  void on_load_page();

  std::condition_variable &get_cv() { return cv_; }

  bool is_on_load_page_required() const { return on_load_page_required_; }

  void notify_all();

private:
  std::condition_variable cv_;
  const bool on_load_page_required_;
};

DECLARE_PTR(bus);

} // namespace kungfu::yijinjing::journal

#endif // KUNGFU_BUS_H