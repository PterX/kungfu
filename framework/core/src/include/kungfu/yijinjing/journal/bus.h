#ifndef KUNGFU_BUS_H
#define KUNGFU_BUS_H
#include <condition_variable>
#include <kungfu/common.h>
namespace kungfu::yijinjing::journal {

class bus {
public:
  explicit bus(const bool on_load_page_required) : on_load_page_required_(on_load_page_required){};

  virtual ~bus() = default;

  void on_load_page();

  [[nodiscard]] bool is_on_load_page_required() const { return on_load_page_required_; }

  void notify_all();

  void consume();

  void produce();

  void wait();

  static void set_trigger_frame_uid(uint64_t frame_uid);

  static uint64_t get_trigger_frame_uid();

private:
  std::condition_variable cv_{};
  std::mutex cv_mutex_{};
  std::atomic<bool> ready_{false};
  const bool on_load_page_required_;
  inline static thread_local uint64_t trigger_frame_uid_ = 0;
};

DECLARE_PTR(bus);
} // namespace kungfu::yijinjing::journal
#endif // KUNGFU_BUS_H