// SPDX-License-Identifier: Apache-2.0

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/cache/cached.h>
#include <kungfu/yijinjing/practice/hero.h>
#include <kungfu/yijinjing/time.h>

using namespace kungfu::rx;
using namespace kungfu::longfist;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::cache;

#define DEFAULT_STORE_VOLUME_BY_INTERVAL 100

namespace kungfu::yijinjing::cache {

cached::cached(const yijinjing::io_device_ptr &io_device, bool bypass_cached)
    : session_builder_(io_device), profile_(io_device->get_locator()), bypass_cached_(bypass_cached),
      ledger_home_location_(yijinjing::practice::make_system_location("service", "ledger", io_device->get_locator())) {
  profile_.setup();
  profile_get_all(profile_, profile_feed_bank_);
}

cached::~cached() {
  {
    std::lock_guard<std::mutex> lock(feed_mutex_);
    m_quit_ = true;
  }

  if (store_states_worker_.joinable()) {
    store_states_worker_.join();
  }

  if (store_profile_worker_.joinable()) {
    store_profile_worker_.join();
  }
}

void cached::restore_profile(const yijinjing::data::location_ptr &location,
                             const yijinjing::journal::writer_ptr &writer) {
  if (not bypass_cached_) {
    profile_store_mutex_.lock();
    try {
      // for config, basket, instruemnts .etc. from user interface
      profile_get_all(profile_, profile_restore_bank_);
    } catch (const std::exception &ex) {
      SPDLOG_ERROR("failed to drain profile db into profile band {} {} {}", location->uid, location->uname, ex.what());
    }
    profile_store_mutex_.unlock();
  }

  feed_mutex_.lock();
  profile_restore_bank_ >> writer;
  profile_feed_bank_ >> writer;
  feed_mutex_.unlock();
}

void cached::restore_states(const yijinjing::data::location_ptr &location,
                            const yijinjing::journal::writer_ptr &writer) {
  if (bypass_cached_) {
    return;
  }

  std::lock_guard<std::mutex> lock(states_store_mutex_);

  try {
    make_cache_shift(location);
    app_states_shift_.at(location->uid) >> writer;
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("failed to write cache {} {} {}", location->uid, location->uname, ex.what());
  }

  if (location->category == category::TD or location->category == category::STRATEGY) {
    for (const auto &other_location : location->locator->list_locations("*", "*", "*", "*")) {
      if (other_location->category == category::SYSTEM) {
        continue;
      }

      for (auto dest : location->locator->list_location_dest_by_db(other_location)) {
        if (dest == location->uid) {
          try {
            ensure_cached_storage(other_location, dest);
            app_states_shift_.at(other_location->uid).restore_to(writer, dest);
          } catch (const std::exception &ex) {
            SPDLOG_ERROR("failed to write cache {} {} {}", other_location->uname, dest, ex.what());
          }
        }
      }
    }
  }

  if (location->uid == ledger_home_location_->uid or
      (location->category == category::SYSTEM and location->group == "node")) {
    for (const auto &other_location : location->locator->list_locations("td", "*", "*", "live")) {
      for (auto dest : location->locator->list_location_dest_by_db(other_location)) {
        try {
          ensure_cached_storage(other_location, dest);
          app_states_shift_.at(other_location->uid).restore_to(writer, dest);
        } catch (const std::exception &ex) {
          SPDLOG_ERROR("failed to write cache {} {} {} for target {}", other_location->uname, dest, ex.what(),
                       location->uname);
        }
      }
    }
  }

  if (location->category == category::SYSTEM and location->group == "node") {
    for (const auto &other_location : location->locator->list_locations("system", "service", "ledger", "live")) {
      for (auto dest : location->locator->list_location_dest_by_db(other_location)) {
        try {
          ensure_cached_storage(other_location, dest);

        } catch (const std::exception &ex) {
          SPDLOG_ERROR("failed to write cache {} {} {} for target {}", other_location->uname, dest, ex.what(),
                       location->uname);
        }
      }
    }
  }
}

void cached::restore(const location_ptr &location, const journal::writer_ptr &writer) {
  restore_profile(location, writer);
  restore_states(location, writer);
}

void cached::clear_cache_shift(const location_ptr &location) {
  if (bypass_cached_) {
    return;
  }

  uint32_t location_uid = location->uid;
  if (app_states_shift_.find(location_uid) == app_states_shift_.end()) {
    SPDLOG_INFO("no location_uid {} in app_states_shift_, no need to clear cache", location->uname);
    return;
  }

  // clear storage_map_ memory, for ensure_storage working fine next time
  app_states_shift_.erase(location_uid);
}

void cached::make_cache_shift(const location_ptr &location) {
  if (bypass_cached_) {
    return;
  }

  locations_.emplace(location->uid, location);
  app_states_shift_.emplace(location->uid, location);
}

void cached::try_ensure_cached_storage(const location_ptr &location, uint32_t dest) {
  if (bypass_cached_) {
    return;
  }

  std::lock_guard<std::mutex> lock(states_store_mutex_);
  ensure_cached_storage(location, dest);
}

void cached::ensure_cached_storage(const location_ptr &location, uint32_t dest) {
  make_cache_shift(location);
  app_states_shift_.at(location->uid).ensure_storage(dest);
}

void cached::cache_reset(const event_ptr &event) {
  if (bypass_cached_) {
    return;
  }

  std::lock_guard<std::mutex> lock(states_store_mutex_);
  auto cache_reset = event->data<CacheReset>();
  auto msg_type = cache_reset.msg_type;
  boost::hana::for_each(StateDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    if (DataType::tag == msg_type) {
      if (app_states_shift_.find(event->source()) != app_states_shift_.end()) {
        app_states_shift_[event->source()] -= typed_event_ptr<DataType>(event);
      }
      if (app_states_shift_.find(event->dest()) != app_states_shift_.end()) {
        app_states_shift_[event->dest()] /= typed_event_ptr<DataType>(event);
      }
    }
  });
}

void cached::feed(const event_ptr &event) {
  std::lock_guard<std::mutex> lock(feed_mutex_);
  feed_profile_data(event, profile_feed_bank_);

  if (not bypass_cached_) {
    feed_state_data(event, states_feed_bank_);
  }
}

void cached::run_store_workers() {
  store_profile_worker_ = std::thread(&cached::do_store_profile_feeds, this);

  if (not bypass_cached_) {
    store_states_worker_ = std::thread(&cached::do_store_states_feeds, this);
  }
}

void cached::do_store_states_feeds() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    store_states_feeds();

    std::lock_guard<std::mutex> lock(feed_mutex_);
    if (m_quit_) {
      break;
    }
  }
}

void cached::do_store_profile_feeds() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    store_profile_feeds();

    std::lock_guard<std::mutex> lock(feed_mutex_);
    if (m_quit_) {
      break;
    }
  }
}

void cached::store_states_feeds() {
  yijinjing::cache::bank tmp_feed_bank = {};

  auto transform_from_state_bank = [&](auto datatypes, uint32_t limit_count = 0) {
    feed_mutex_.lock();
    auto count = 0;
    boost::hana::for_each(datatypes, [&](auto it) {
      if (limit_count != 0 && count >= limit_count) {
        return;
      }
      using DataType = typename decltype(+boost::hana::second(it))::type;
      if (DataType::tag == Instrument::tag) {
        return;
      }

      auto hana_type = boost::hana::type_c<DataType>;
      using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
      auto &feed_map = const_cast<FeedMap &>(states_feed_bank_[hana_type]);
      for (auto it = feed_map.begin(); it != feed_map.end();) {
        tmp_feed_bank << it->second;
        it = feed_map.erase(it);
        count++;
      }
    });
    feed_mutex_.unlock();
    return count;
  };

  auto store_map = [&](auto &feed_map, auto type_name) {
    if (feed_map.size() == 0) {
      return;
    }

    auto iter = feed_map.begin();
    while (iter != feed_map.end()) {
      auto &s = iter->second;
      auto source_id = s.source;
      auto dest_id = s.dest;
      if (app_states_shift_.find(source_id) != app_states_shift_.end()) {
        states_store_mutex_.lock();
        try {
          app_states_shift_.at(source_id) << s;
          SPDLOG_TRACE("cache [feed] source {} dest {} {} data {}", source_id, dest_id, type_name.c_str(),
                       s.data.to_string());
          iter++;
        } catch (const std::exception &e) {
          SPDLOG_ERROR("Unexpected exception by handle_cached_feeds {}", e.what());
        }
        states_store_mutex_.unlock();
      } else {
        iter++;
      }
    }
    feed_map.clear();
  };

  auto store_trading_data_start_time = time::now_in_nano();
  auto trading_data_count = transform_from_state_bank(TradingDataTypes);
  boost::hana::for_each(TradingDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    auto hana_type = boost::hana::type_c<DataType>;
    using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
    auto &feed_map = const_cast<FeedMap &>(tmp_feed_bank[hana_type]);
    store_map(feed_map, DataType::type_name);
  });
  auto store_trading_data_end_time = time::now_in_nano();
  SPDLOG_DEBUG("store trading data take {}ns, count {}", store_trading_data_end_time - store_trading_data_start_time,
               trading_data_count);

  auto store_others_start_time = time::now_in_nano();
  auto others_data_count = transform_from_state_bank(StateDataTypes, DEFAULT_STORE_VOLUME_BY_INTERVAL);
  boost::hana::for_each(StateDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    auto hana_type = boost::hana::type_c<DataType>;
    using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
    auto &feed_map = const_cast<FeedMap &>(tmp_feed_bank[hana_type]);
    store_map(feed_map, DataType::type_name);
  });
  auto store_others_end_time = time::now_in_nano();
  SPDLOG_DEBUG("store others data take {}ns, count {}", store_others_end_time - store_others_start_time,
               others_data_count);
}

void cached::store_profile_feeds() {
  // there are important info like locations in profile, every app register need these info, so do not clear profile
  // bank;
  feed_mutex_.lock();
  ProfileStateBank tmp_profile_bank = profile_feed_bank_;
  profile_feed_bank_.clear();
  feed_mutex_.unlock();

  auto count = 0;
  auto store_profile_data_start_time = time::now_in_nano();
  boost::hana::for_each(ProfileDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    auto hana_type = boost::hana::type_c<DataType>;

    using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
    auto &feed_map = const_cast<FeedMap &>(tmp_profile_bank[hana_type]);

    if (feed_map.size() != 0) {
      auto iter = feed_map.begin();
      while (iter != feed_map.end()) {
        const auto &s = iter->second;
        profile_store_mutex_.lock();
        try {
          profile_ << s;
          count++;
          SPDLOG_TRACE("cache [profile] {} data {}", DataType::type_name.c_str(), s.data.to_string());
          iter++;
        } catch (const std::exception &e) {
          SPDLOG_ERROR("Unexpected exception by handle_profile_feeds {}", e.what());
        }
        profile_store_mutex_.unlock();
      }
    }
  });
  auto store_profile_data_end_time = time::now_in_nano();
  SPDLOG_DEBUG("store profile data take {}ns, count {}", store_profile_data_end_time - store_profile_data_start_time,
               count);
}

void cached::open_session(const location_ptr &location, int64_t open_time) {
  if (bypass_cached_) {
    return;
  }

  session_builder_.open_session(location, open_time);
}

void cached::close_session(const location_ptr &location, int64_t close_time) {
  if (bypass_cached_) {
    return;
  }

  session_builder_.close_session(location, close_time);
}

void cached::close_all_sessions(int64_t close_time) { return session_builder_.close_all_sessions(close_time); }

index::SessionMap &cached::get_all_sessions() { return session_builder_.get_all_sessions(); }

int64_t cached::find_last_active_time(const location_ptr &location) {
  if (bypass_cached_) {
    return yijinjing::time::now_in_nano();
  }

  return session_builder_.find_last_active_time(location);
}

void cached::update_session(const journal::frame_ptr &frame) {
  if (bypass_cached_) {
    return;
  }
  session_builder_.update_session(frame);
}

} // namespace kungfu::yijinjing::cache