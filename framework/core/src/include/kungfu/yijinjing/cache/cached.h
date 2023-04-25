// SPDX-License-Identifier: Apache-2.0

#ifndef KUNGFU_CACHED_H
#define KUNGFU_CACHED_H

#include <kungfu/yijinjing/cache/profile.h>
#include <kungfu/yijinjing/cache/runtime.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/log.h>

namespace kungfu::yijinjing::cache {

using ProfileDataTypesType = decltype(longfist::ProfileDataTypes);
using ProfileStateMapType = decltype(longfist::build_state_map(longfist::ProfileDataTypes));
typedef yijinjing::cache::typed_bank<ProfileDataTypesType, ProfileStateMapType> ProfileStateBank;

class cached {
public:
  explicit cached(yijinjing::data::locator_ptr locator);

  ~cached();

  template <typename DataType> std::vector<DataType> get_all(const DataType &) { return profile_.get_all(DataType{}); }

  void restore(const yijinjing::data::location_ptr &location, yijinjing::journal::writer_ptr &writer);

  void clear_cache_shift(const yijinjing::data::location_ptr &location);

  void make_cache_shift(const yijinjing::data::location_ptr &location);

  void ensure_cached_storage(const yijinjing::data::location_ptr &location, uint32_t dest);

  void cache_reset(const event_ptr &event);

  void feed(const event_ptr &event);

  void run_feeds_worker();

  void do_store_feeds();

  void store_cached_feeds();

  void store_profile_feeds();

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
  yijinjing::cache::profile profile_;
  std::unordered_map<uint32_t, yijinjing::data::location_ptr> locations_ = {};
  ProfileStateBank profile_bank_ = ProfileStateBank(longfist::ProfileDataTypes);
  std::unordered_map<uint32_t, yijinjing::cache::shift> app_cache_shift_ = {};
  yijinjing::cache::bank feed_bank_;
  std::thread feed_worker_;
  std::mutex feed_mutex_;
  std::mutex db_mutex_;
  bool m_quit_ = false;

  static constexpr auto profile_get_all = [](auto &profile, auto &receiver) {
    boost::hana::for_each(longfist::ProfileDataTypes, [&](auto it) {
      using DataType = typename decltype(+boost::hana::second(it))::type;
      for (const auto &data : profile.get_all(DataType{})) {
        auto s = state(0, 0, 0, data);
        receiver << s;
      }
    });
  };
};

} // namespace kungfu::yijinjing::cache

#endif // KUNGFU_CACHED_H
