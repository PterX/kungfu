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
#define LOW_LATENCY_STORE_VOLUME_BY_INTERVAL 10

namespace kungfu::yijinjing::cache {

cached::cached(locator_ptr locator) : profile_(locator) {
  profile_.setup();
  profile_get_all(profile_, profile_bank_);
}

cached::~cached() {

  {
    std::lock_guard<std::mutex> lock(feed_mutex_);
    m_quit_ = true;
  }

  feed_worker_.join();
}

void cached::restore(const location_ptr &location, const journal::writer_ptr &writer) {

  if (app_cache_shift_.find(location->uid) == app_cache_shift_.end()) {
    make_cache_shift(location);
  }

  try {
    cache_store_mutex_.lock();
    app_cache_shift_.at(location->uid) >> writer;
    cache_store_mutex_.unlock();
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("failed to write cache {} {} {}", location->uid, location->uname, ex.what());
  }

  try {
    profile_store_mutex_.lock();
    feed_mutex_.lock();
    profile_get_all(profile_, profile_bank_);
    profile_bank_ >> writer;
    profile_store_mutex_.unlock();
    feed_mutex_.unlock();
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("failed to write profile info {} {} {}", location->uid, location->uname, ex.what());
  }
}

void cached::clear_cache_shift(const location_ptr &location) {
  uint32_t location_uid = location->uid;
  if (app_cache_shift_.find(location_uid) == app_cache_shift_.end()) {
    SPDLOG_INFO("no location_uid {} in app_cache_shift_, no need to clear cache", location->uname);
    return;
  }

  // clear storage_map_ memory, for ensure_storage working fine next time
  app_cache_shift_.erase(location_uid);
}

void cached::make_cache_shift(const location_ptr &location) {
  locations_.emplace(location->uid, location);
  app_cache_shift_.emplace(location->uid, location);
}

void cached::ensure_cached_storage(const location_ptr &location, uint32_t dest) {
  std::lock_guard<std::mutex> lock(cache_store_mutex_);
  make_cache_shift(location);
  app_cache_shift_.at(location->uid).ensure_storage(dest);
}

void cached::cache_reset(const event_ptr &event) {
  std::lock_guard<std::mutex> lock(cache_store_mutex_);
  auto cache_reset = event->data<CacheReset>();
  auto msg_type = cache_reset.msg_type;
  boost::hana::for_each(StateDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    if (DataType::tag == msg_type) {
      if (app_cache_shift_.find(event->source()) != app_cache_shift_.end()) {
        app_cache_shift_[event->source()] -= typed_event_ptr<DataType>(event);
      }
      if (app_cache_shift_.find(event->dest()) != app_cache_shift_.end()) {
        app_cache_shift_[event->dest()] /= typed_event_ptr<DataType>(event);
      }
    }
  });
}

void cached::feed(const event_ptr &event) {
  feed_state_data(event, feed_bank_);
  feed_profile_data(event, profile_bank_);
}

void cached::run_feeds_worker() { feed_worker_ = std::thread(&cached::do_store_feeds, this); }

void cached::do_store_feeds() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    store_cached_feeds();
    store_profile_feeds();

    std::lock_guard<std::mutex> lock(feed_mutex_);
    if (m_quit_) {
      break;
    }
  }
}

void cached::store_cached_feeds() {
  feed_mutex_.lock();
  yijinjing::cache::bank tmp_feed_bank = feed_bank_;
  feed_bank_.clear();
  feed_mutex_.unlock();

  boost::hana::for_each(StateDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    auto hana_type = boost::hana::type_c<DataType>;

    using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
    auto &feed_map = const_cast<FeedMap &>(tmp_feed_bank[hana_type]);

    if (feed_map.size() != 0) {
      auto iter = feed_map.begin();
      while (iter != feed_map.end()) {
        auto &s = iter->second;
        auto source_id = s.source;
        auto dest_id = s.dest;
        if (app_cache_shift_.find(source_id) != app_cache_shift_.end()) {
          try {
            app_cache_shift_.at(source_id) << s;
            SPDLOG_TRACE("cache [feed] source {} dest {} {} data {}", source_id, dest_id, DataType::type_name.c_str(),
                         s.data.to_string());
          } catch (const std::exception &e) {
            SPDLOG_ERROR("Unexpected exception by handle_cached_feeds {}", e.what());
            break;
          }

          iter = feed_map.erase(iter);
        } else {
          iter++;
        }
      }
    }
  });
}

void cached::store_profile_feeds() {
  feed_mutex_.lock();
  ProfileStateBank tmp_profile_bank = profile_bank_;
  profile_bank_.clear();
  feed_mutex_.unlock();

  boost::hana::for_each(ProfileDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    auto hana_type = boost::hana::type_c<DataType>;

    using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
    auto &feed_map = const_cast<FeedMap &>(profile_bank_[hana_type]);

    if (feed_map.size() != 0) {
      auto iter = feed_map.begin();
      while (iter != feed_map.end()) {
        const auto &s = iter->second;
        try {
          profile_ << s;
          SPDLOG_TRACE("cache [profile] {} data {}", DataType::type_name.c_str(), s.data.to_string());
        } catch (const std::exception &e) {
          SPDLOG_ERROR("Unexpected exception by handle_profile_feeds {}", e.what());
          break;
        }

        iter = feed_map.erase(iter);
      }
    }
  });
}

} // namespace kungfu::yijinjing::cache