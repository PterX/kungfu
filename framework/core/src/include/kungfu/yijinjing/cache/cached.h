// SPDX-License-Identifier: Apache-2.0

#ifndef KUNGFU_CACHED_H
#define KUNGFU_CACHED_H

#include <kungfu/yijinjing/cache/profile.h>
#include <kungfu/yijinjing/cache/runtime.h>
#include <kungfu/yijinjing/index/session.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/log.h>

namespace kungfu::yijinjing::cache {

using ProfileDataTypesType = decltype(longfist::ProfileDataTypes);
using ProfileStateMapType = decltype(longfist::build_state_map(longfist::ProfileDataTypes));
typedef yijinjing::cache::typed_bank<ProfileDataTypesType, ProfileStateMapType> ProfileStateBank;

class cached {
public:
  explicit cached(const yijinjing::io_device_ptr &io_device, bool bypass_cached = false);

  ~cached();

  template <typename DataType> std::vector<DataType> get_all(const DataType &) { return profile_.get_all(DataType{}); }

  void restore_profile(const yijinjing::data::location_ptr &location, const yijinjing::journal::writer_ptr &writer);

  void restore_states(const yijinjing::data::location_ptr &location, const yijinjing::journal::writer_ptr &writer);

  void restore(const yijinjing::data::location_ptr &location, const yijinjing::journal::writer_ptr &writer);

  void clear_cache_shift(const yijinjing::data::location_ptr &location);

  void make_cache_shift(const yijinjing::data::location_ptr &location);

  void ensure_cached_storage(const yijinjing::data::location_ptr &location, uint32_t dest);

  void cache_reset(const event_ptr &event);

  void feed(const event_ptr &event);

  template <typename DataType>
  std::enable_if_t<longfist::is_profile_data<DataType>()> feed_profile(const DataType &data) {
    std::lock_guard<std::mutex> lock(feed_mutex_);
    auto s = state(0, 0, 0, data);
    profile_feed_bank_ << s;
  }

  void run_store_workers();

  void do_store_states_feeds();

  void do_store_profile_feeds();

  void store_states_feeds();

  void store_profile_feeds();

  void open_session(const data::location_ptr &location, int64_t open_time);

  void close_session(const data::location_ptr &location, int64_t close_time);

  void close_all_sessions(int64_t close_time);

  int64_t find_last_active_time(const data::location_ptr &source_location);

  void update_session(const journal::frame_ptr &frame);

  yijinjing::index::SessionMap &get_all_sessions();

  static constexpr auto feed_profile_data = [](const event_ptr &event, auto &receiver) {
    boost::hana::for_each(longfist::ProfileDataTypes, [&](auto it) {
      using DataType = typename decltype(+boost::hana::second(it))::type;
      if (DataType::tag == event->msg_type()) {
        receiver << typed_event_ptr<DataType>(event);
      }
    });
  };

  static constexpr auto feed_state_data = [](const event_ptr &event, auto &receiver) {
    boost::hana::for_each(longfist::StateDataTypes, [&](auto it) {
      using DataType = typename decltype(+boost::hana::second(it))::type;
      if (DataType::tag == event->msg_type()) {
        receiver << typed_event_ptr<DataType>(event);
      }
    });
  };

  static constexpr auto feed_trading_data = [](const event_ptr &event, auto &receiver) {
    boost::hana::for_each(longfist::TradingDataTypes, [&](auto it) {
      using DataType = typename decltype(+boost::hana::second(it))::type;
      if (DataType::tag == event->msg_type()) {
        receiver << typed_event_ptr<DataType>(event);
      }
    });
  };

private:
  index::session_builder session_builder_;
  std::unordered_map<uint32_t, yijinjing::data::location_ptr> locations_ = {};
  yijinjing::cache::profile profile_;
  ProfileStateBank profile_feed_bank_ = ProfileStateBank(longfist::ProfileDataTypes);
  std::unordered_map<uint32_t, yijinjing::cache::shift> app_states_shift_ = {};
  yijinjing::cache::bank states_feed_bank_;
  const bool bypass_cached_;
  std::thread store_states_worker_;
  std::thread store_profile_worker_;
  std::mutex feed_mutex_;
  std::mutex states_store_mutex_;
  std::mutex profile_store_mutex_;
  bool m_quit_ = false;

  static constexpr auto profile_get_all = [](auto &profile, auto &receiver) {
    boost::hana::for_each(longfist::ProfileDataTypes, [&](auto it) {
      using DataType = typename decltype(+boost::hana::second(it))::type;
      try {

        for (const auto &data : profile.get_all(DataType{})) {
          auto s = state(0, 0, 0, data);
          receiver << s;
        }
      } catch (const std::exception &e) {
        SPDLOG_ERROR("Unexpected exception by profile_get_all {}", e.what());
      }
    });
  };

  template <typename DataType>
  static constexpr auto profile_get_by_type = [](auto &profile, auto &receiver) {
    try {
      for (const auto &data : profile.get_all(DataType{})) {
        auto s = state(0, 0, 0, data);
        receiver << s;
      }
    } catch (const std::exception &e) {
      SPDLOG_ERROR("Unexpected exception by profile_get_all {}", e.what());
    }
  };
};

} // namespace kungfu::yijinjing::cache

#endif // KUNGFU_CACHED_H
