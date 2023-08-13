// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2019-06-01.
//

#ifndef KUNGFU_APPRENTICE_H
#define KUNGFU_APPRENTICE_H

#include <kungfu/yijinjing/cache/runtime.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/practice/hero.h>
#include <kungfu/yijinjing/time.h>

namespace kungfu::yijinjing::practice {
class apprentice;

class cleaner {
public:
  explicit cleaner(yijinjing::practice::apprentice &app);

  virtual ~cleaner();

  void on_react();

private:
  yijinjing::practice::apprentice &app_;
  std::thread cleaning_worker_;
  std::mutex cv_mutex_;
  std::mutex quite_mutex_;
  bool m_quit_ = false;

  void do_clean();

  [[nodiscard]] bool is_cleaner_worker_required() const;
};

class apprentice : public hero {
public:
  explicit apprentice(yijinjing::data::location_ptr home, bool low_latency = false);

  bool is_started() const;

  void pause();

  uint32_t get_master_command_uid() const;

  int64_t get_last_active_time() const;

  int64_t get_checkin_time() const;

  const cache::bank &get_state_bank() const;

  void request_read_from(int64_t trigger_time, uint32_t source_id, int64_t from_time);

  void request_read_from_public(int64_t trigger_time, uint32_t source_id, int64_t from_time);

  void request_read_from_sync(int64_t trigger_time, uint32_t source_id, int64_t from_time);

  void request_read_from_source_to_dest(int64_t trigger_time, const yijinjing::data::location_ptr &source_location,
                                        uint32_t dest_id);

  void request_write_to(int64_t trigger_time, uint32_t dest_id);

  void request_write_to_band(int64_t trigger_time, const yijinjing::data::location_ptr &location,
                             uint32_t page_size = 0);

  uint32_t request_band(const std::string &band_name, uint32_t page_size = 0);

  void add_timer(int64_t nanotime, const std::function<void(const event_ptr &)> &callback);

  void add_time_interval(int64_t nanotime, const std::function<void(const event_ptr &)> &callback);

  template <typename DataType>
  void write_to(int64_t trigger_time, DataType &data, uint32_t dest_id = yijinjing::data::location::PUBLIC) {
    get_writer(dest_id)->write(trigger_time, data);
  }

  bool release_page();

  template <class DataType> std::string make_nano_msg(uint32_t source, uint32_t dest, const DataType &data) const {
    auto now = this->now();
    nlohmann::json request;
    request["data_type"] = int8_t(longfist::enums::FrameDataType::Json);
    request["msg_type"] = DataType::tag;
    request["gen_time"] = now;
    request["trigger_time"] = now;
    request["initial_source"] = get_home_uid();
    request["source"] = source;
    request["dest"] = dest;
    request["data"] = nlohmann::json::parse(data.to_string());
    return request.dump();
  }

protected:
  cache::bank state_bank_;

  friend void add_location(yijinjing::practice::apprentice &app, const yijinjing::data::location_ptr &location) {
    app.add_location(app.now(), location);
  }

  void react() override;

  void on_active() override;

  void on_frame() override;

  virtual void on_react();

  virtual void on_start();

  void on_register(int64_t trigger_time, const longfist::types::Register &register_data);

  void on_deregister(const event_ptr &event);

  void on_read_from(const event_ptr &event);

  void on_read_from_public(const event_ptr &event);

  void on_read_from_sync(const event_ptr &event);

  void on_request_read_from_others(const event_ptr &event);

  virtual void on_write_to(const event_ptr &event);

  void on_write_to_band(const event_ptr &event);

  [[maybe_unused]] int get_observer_recv_timeout() const;

  void reader_join(uint32_t source_id, uint32_t dest_id, int64_t from_time);

  std::function<rx::observable<event_ptr>(rx::observable<event_ptr>)> timer(int64_t nanotime) {
    int32_t timer_usage_count = timer_usage_count_;
    int64_t duration_ns = nanotime - now();
    auto writer = get_writer(get_master_command_uid());
    longfist::types::TimeRequest &r = writer->open_data<longfist::types::TimeRequest>(now());
    r.id = timer_usage_count;
    r.base_time = now();
    r.duration = duration_ns;
    r.repeat = 1;
    r.location_uid = get_home_uid();
    writer->close_data();
    timer_checkpoints_[timer_usage_count] = now();
    timer_requests_.insert_or_assign(timer_usage_count, r);
    timer_usage_count_++;
    return [&, duration_ns, timer_usage_count](const rx::observable<event_ptr> &src) {
      return events_ | rx::filter([&, duration_ns, timer_usage_count](const event_ptr &event) {
               if (event->msg_type() == longfist::types::Time::tag &&
                   event->gen_time() > timer_checkpoints_[timer_usage_count] + duration_ns) {
                 timer_requests_.erase(timer_usage_count);
                 return true;
               } else {
                 return false;
               };
             }) |
             rx::first();
    };
  }

  template <typename Duration, typename Enabled = rx::is_duration<Duration>>
  std::function<rx::observable<event_ptr>(rx::observable<event_ptr>)> time_interval(Duration &&d) {
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    int32_t timer_usage_count = timer_usage_count_;
    auto writer = get_writer(get_master_command_uid());
    longfist::types::TimeRequest &r = writer->open_data<longfist::types::TimeRequest>(now());
    r.id = timer_usage_count;
    r.base_time = now();
    r.duration = duration_ns;
    r.repeat = 1;
    r.location_uid = get_home_uid();
    writer->close_data();
    timer_checkpoints_[timer_usage_count] = now();
    timer_requests_.insert_or_assign(timer_usage_count, r);
    timer_usage_count_++;
    return [&, duration_ns, timer_usage_count](const rx::observable<event_ptr> &src) {
      return events_ | rx::filter([&, duration_ns, timer_usage_count](const event_ptr &event) {
               if (event->msg_type() == longfist::types::Time::tag &&
                   event->gen_time() > timer_checkpoints_[timer_usage_count] + duration_ns) {
                 auto writer = get_writer(get_master_command_uid());
                 longfist::types::TimeRequest &r = writer->open_data<longfist::types::TimeRequest>(now());
                 r.id = timer_usage_count;
                 r.base_time = now();
                 r.duration = duration_ns;
                 r.repeat = 1;
                 r.location_uid = get_home_uid();
                 writer->close_data();
                 timer_checkpoints_[timer_usage_count] = now();
                 timer_requests_.insert_or_assign(timer_usage_count, r);
                 return true;
               } else {
                 return false;
               }
             });
    };
  }

  template <typename Duration, typename Enabled = rx::is_duration<Duration>>
  std::function<rx::observable<event_ptr>(rx::observable<event_ptr>)> timeout(Duration &&d) {
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    int32_t timer_usage_count = timer_usage_count_;
    auto writer = get_writer(get_master_command_uid());
    longfist::types::TimeRequest &r = writer->open_data<longfist::types::TimeRequest>(now());
    r.id = timer_usage_count;
    r.base_time = now();
    r.duration = duration_ns;
    r.repeat = 1;
    r.location_uid = get_home_uid();
    writer->close_data();
    timer_checkpoints_[timer_usage_count] = now();
    timer_requests_.insert_or_assign(timer_usage_count, r);
    timer_usage_count_++;
    return [&, duration_ns, timer_usage_count](const rx::observable<event_ptr> &src) {
      return (src | rx::filter([&, duration_ns, timer_usage_count](const event_ptr &event) {
                if (event->msg_type() != longfist::types::Time::tag) {
                  auto writer = get_writer(get_master_command_uid());
                  longfist::types::TimeRequest &r = writer->open_data<longfist::types::TimeRequest>(now());
                  r.id = timer_usage_count;
                  r.base_time = now();
                  r.duration = duration_ns;
                  r.repeat = 1;
                  r.location_uid = get_home_uid();
                  writer->close_data();
                  timer_checkpoints_[timer_usage_count] = now();
                  timer_requests_.insert_or_assign(timer_usage_count, r);
                  return true;
                } else {
                  return false;
                }
              }))
          .merge(events_ | rx::filter([&, duration_ns, timer_usage_count](const event_ptr &event) {
                   if (event->gen_time() > timer_checkpoints_[timer_usage_count] + duration_ns) {
                     timer_requests_.erase(timer_usage_count);
                     throw rx::timeout_error("timeout");
                   }
                   timer_requests_.erase(timer_usage_count);
                   return false;
                 }));
    };
  }

private:
  bool started_ = false;
  int64_t last_active_time_ = INT64_MIN;
  int64_t checkin_time_ = INT64_MIN;
  int32_t timer_usage_count_ = 0;
  yijinjing::practice::cleaner cleaner_;
  std::unordered_map<int, int64_t> timer_checkpoints_ = {};
  std::unordered_map<int, longfist::types::TimeRequest> timer_requests_ = {};

  void checkin();

  void expect_start();

  template <typename DataType> void do_read_from(const event_ptr &event, uint32_t dest_id) {
    const DataType &request = event->data<DataType>();
    reader_->join(get_location(request.source_id), dest_id, request.from_time);
  }

  static void reset_time(const longfist::types::TimeReset &time_reset);
};

DECLARE_PTR(apprentice)
} // namespace kungfu::yijinjing::practice

#endif // KUNGFU_APPRENTICE_H
