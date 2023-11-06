// SPDX-License-Identifier: Apache-2.0

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/cache/cached.h>
#include <kungfu/yijinjing/time.h>

using namespace kungfu::rx;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::longfist;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::cache;

// https://sqlite.org/limits.html
// The maximum number of bytes in the text of an SQL statement is limited to SQLITE_MAX_SQL_LENGTH which defaults to
// 1,000,000,000.
#define DEFAULT_STORE_VOLUME_BY_INTERVAL 1000

namespace kungfu::yijinjing::cache {

cached::cached(locator_ptr locator, mode m, bool low_latency, const std::string &arguments)
    : apprentice(location::make_shared(m, category::SYSTEM, "service", "cached", std::move(locator)), low_latency),
      profile_(get_locator()) {
  profile_.setup();
  profile_get_all(profile_, profile_bank_);
}

void cached::on_react() {
  events_ | is(Location::tag) | $$(on_location(event));
  events_ | is(Register::tag) | $$(register_triggger_clear_cache_shift(event->data<Register>()));
  events_ | is(Register::tag) | $$(register_trigger_listen_public(event->gen_time(), event->data<Register>()));
  events_ | is(CachedPause::tag) | $$(switch_feed_storage(true));
  events_ | is(CachedResume::tag) | $$(switch_feed_storage(false));
  events_ | is(RequestCached::tag) | $([&](const event_ptr &event) {
    auto source_id = event->source();

    SPDLOG_INFO("get RequestCached from {}", get_location_uname(source_id));

    if (locations_.find(source_id) == locations_.end()) {
      SPDLOG_ERROR("no location {} in locations_", get_location_uname(source_id));
      return;
    }

    const auto &location = locations_.at(source_id);
    app_cache_shift_.try_emplace(source_id, location);
    auto cached_writer = get_writer(source_id);

    try {
      app_cache_shift_.at(source_id) >> cached_writer;
    } catch (const std::exception &ex) {
      SPDLOG_ERROR("failed to write cache {} {} {}", source_id, get_location_uname(source_id), ex.what());
    }

    if (location->category == category::TD or location->category == category::STRATEGY) {
      for (const auto &other_location : location->locator->list_locations("*", "*", "*", "live")) {
        if (other_location->category == category::SYSTEM) {
          continue;
        }

        for (auto dest : location->locator->list_location_dest_by_db(other_location)) {
          if (dest == location->uid) {
            try {
              make_cache_shift(other_location->uid, dest);
              app_cache_shift_.at(other_location->uid).restore_to(cached_writer, dest);
            } catch (const std::exception &ex) {
              SPDLOG_ERROR("failed to write cache {} {} {} for td or strategy", other_location->uname, dest, ex.what());
            }
          }
        }
      }
    }

    if (location->uid == get_ledger_home_location()->uid or
        (location->category == category::SYSTEM and location->group == "node")) {
      for (const auto &other_location : location->locator->list_locations("td", "*", "*", "live")) {
        for (auto dest : location->locator->list_location_dest_by_db(other_location)) {
          try {
            make_cache_shift(other_location->uid, dest);
            app_cache_shift_.at(other_location->uid).restore_to(cached_writer, dest);
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
            make_cache_shift(other_location->uid, dest);
            app_cache_shift_.at(other_location->uid).restore_to(cached_writer, dest);
          } catch (const std::exception &ex) {
            SPDLOG_ERROR("failed to write cache {} {} {} for target {}", other_location->uname, dest, ex.what(),
                         location->uname);
          }
        }
      }
    }

    try {
      profile_get_all(profile_, profile_bank_);
      profile_bank_ >> cached_writer;
    } catch (const std::exception &ex) {
      SPDLOG_ERROR("failed to write profile info {} {} {}", source_id, get_location_uname(source_id), ex.what());
    }

    mark_request_cached_done(source_id);
  });
}

void cached::on_start() {
  events_ | is(Channel::tag) | $$(inspect_channel(event->gen_time(), event->data<Channel>()));
  events_ | is(CacheReset::tag) | $$(on_cache_reset(event));
  events_ | instanceof <journal::frame>() | filter([&](const event_ptr &event) {
                         auto source_id = event->source();
                         return source_id != master_home_location_->uid and source_id != get_master_command_uid();
                       }) | $$(feed(event));

  add_time_interval(time_unit::NANOSECONDS_PER_MILLISECOND * 1000, [&](auto e) {
    handle_cached_feeds(DEFAULT_STORE_VOLUME_BY_INTERVAL);
    handle_profile_feeds(DEFAULT_STORE_VOLUME_BY_INTERVAL);
  });
}

void cached::on_active() {
  handle_cached_feeds(DEFAULT_STORE_VOLUME_BY_INTERVAL);
  handle_profile_feeds(DEFAULT_STORE_VOLUME_BY_INTERVAL);
}

void cached::mark_request_cached_done(uint32_t dest_id) {
  auto writer = get_writer(get_master_command_uid());
  RequestCachedDone &rcd = writer->open_data<RequestCachedDone>();
  rcd.dest_id = dest_id;
  writer->close_data();
}

void cached::handle_cached_feeds(int store_volume_every_loop) {
  if (storage_pause_) {
    return;
  }

  int stored_controller = 0;
  auto store_states_start_time = time::now_in_nano();
  location_bank tmp_location_bank = {};

  auto clear_map = [&](auto &feed_map, auto type_name) {
    auto iter = feed_map.begin();
    while (iter != feed_map.end() and stored_controller <= store_volume_every_loop) {
      if (app_cache_shift_.find(iter->second.source) != app_cache_shift_.end()) {
        tmp_location_bank << iter->second;
        iter = feed_map.erase(iter);
        stored_controller++;
      } else {
        iter++;
      }
    }
  };

  boost::hana::for_each(TradingDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    auto hana_type = boost::hana::type_c<DataType>;
    using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
    auto &feed_map = const_cast<FeedMap &>(feed_bank_[hana_type]);
    clear_map(feed_map, DataType::type_name);
  });

  boost::hana::for_each(StateDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    auto hana_type = boost::hana::type_c<DataType>;
    using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
    auto &feed_map = const_cast<FeedMap &>(feed_bank_[hana_type]);
    clear_map(feed_map, DataType::type_name);
  });

  auto &location_bank_map = tmp_location_bank.get_map();

  std::for_each(location_bank_map.begin(), location_bank_map.end(), [&](auto &pair) {
    uint32_t source = pair.first >> 32u;
    uint32_t dest = pair.first & 0xFFFFFFFF;
    auto &state_bank = pair.second;

    boost::hana::for_each(StateDataTypes, [&](auto it) {
      using DataType = typename decltype(+boost::hana::second(it))::type;
      auto hana_type = boost::hana::type_c<DataType>;
      using StateMap = std::unordered_map<uint64_t, state<DataType>>;

      auto &state_map = const_cast<StateMap &>(state_bank[hana_type]);
      std::vector<DataType> tmp_state_vector = {};
      for (const auto &s : state_map) {
        tmp_state_vector.push_back(s.second.data);
      }

      if (tmp_state_vector.size() <= 0) {
        return;
      }

      try {
        app_cache_shift_.at(source).replace_range(dest, tmp_state_vector);
        SPDLOG_TRACE("cache [state] {} size {}", DataType::type_name.c_str(), tmp_state_vector.size());
      } catch (const std::exception &e) {
        SPDLOG_ERROR("Unexpected exception by handle_cached_feeds {}", e.what());
      }
    });
  });

  auto store_states_end_time = time::now_in_nano();
  SPDLOG_TRACE("store state data take {}ns, count {}", store_states_end_time - store_states_start_time,
               stored_controller);
}

void cached::handle_profile_feeds(int store_volume_every_loop) {
  if (storage_pause_) {
    return;
  }

  int stored_controller = 0;
  auto store_profiles_start_time = time::now_in_nano();

  boost::hana::for_each(ProfileDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;

    if (stored_controller > store_volume_every_loop) {
      return;
    }

    auto hana_type = boost::hana::type_c<DataType>;
    using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
    auto &feed_map = const_cast<FeedMap &>(profile_bank_[hana_type]);
    std::vector<DataType> tmp_profile_vector = {};
    for (const auto &s : feed_map) {
      tmp_profile_vector.push_back(s.second.data);
      stored_controller++;
    }
    feed_map.clear();

    if (tmp_profile_vector.size() <= 0) {
      return;
    }

    try {
      profile_.replace_range(tmp_profile_vector);
      SPDLOG_TRACE("cache [profile] {} size {}", DataType::type_name.c_str(), tmp_profile_vector.size());
    } catch (const std::exception &e) {
      SPDLOG_ERROR("Unexpected exception by handle_profile_feeds {}", e.what());
    }
  });

  auto store_profiles_end_time = time::now_in_nano();
  SPDLOG_TRACE("store profile data take {}ns, count {}", store_profiles_end_time - store_profiles_start_time,
               stored_controller);
}

void cached::on_location(const event_ptr &event) { profile_bank_ << typed_event_ptr<Location>(event); }

void cached::inspect_channel(int64_t trigger_time, const Channel &channel) {
  if (channel.source_id != get_live_home_uid() and channel.dest_id != get_live_home_uid()) {
    reader_join(channel.source_id, channel.dest_id, trigger_time);
    make_cache_shift(channel.source_id, channel.dest_id);
  }
}

void cached::make_cache_shift(uint32_t source_id, uint32_t dest_id) {
  if (locations_.find(source_id) == locations_.end()) {
    SPDLOG_ERROR("no source {} in locations_", get_location_uname(source_id));
    return;
  }

  // if (not is_location_live(source_id)) {
  //   SPDLOG_ERROR("no source {} in registry_", get_location_uname(source_id));
  //   return;
  // }

  const location_ptr &location = locations_.at(source_id);
  app_cache_shift_.emplace(source_id, location);
  ensure_cached_storage(source_id, dest_id);
}

void cached::register_trigger_listen_public(int64_t gen_time, const Register &register_data) {
  auto app_uid = register_data.location_uid;
  auto app_location = get_location(app_uid);

  if (app_location->category != category::TD) {
    return;
  }

  // only public no sync
  reader_->join(app_location, location::PUBLIC, gen_time);
  make_cache_shift(app_uid, location::PUBLIC);
  SPDLOG_INFO("resume {} connection from {}", get_location_uname(app_uid), time::strftime(gen_time));
}

void cached::register_triggger_clear_cache_shift(const Register &register_data) {
  uint32_t location_uid = register_data.location_uid;
  if (app_cache_shift_.find(location_uid) == app_cache_shift_.end()) {
    SPDLOG_INFO("no location_uid {} in app_cache_shift_, no need to clear cache", get_location_uname(location_uid));
    return;
  }

  // clear storage_map_ memory, for ensure_storage working fine next time
  app_cache_shift_.erase(location_uid);
}

void cached::on_cache_reset(const event_ptr &event) {
  auto msg_type = event->data<CacheReset>().msg_type;
  boost::hana::for_each(StateDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    if (DataType::tag == msg_type) {
      app_cache_shift_[event->source()] -= typed_event_ptr<DataType>(event);
      app_cache_shift_[event->dest()] /= typed_event_ptr<DataType>(event);
    }
  });
}

void cached::ensure_cached_storage(uint32_t source_id, uint32_t dest_id) {
  if (app_cache_shift_.find(source_id) == app_cache_shift_.end()) {
    SPDLOG_ERROR("no source {} in app_cache_shift_", get_location_uname(source_id));
    return;
  }
  app_cache_shift_.at(source_id).ensure_storage(dest_id);
}

void cached::feed(const event_ptr &event) {
  if (event->msg_type() != Instrument::tag and event->msg_type() != InstrumentFactor::tag and
      get_location(event->source())->category == category::MD) {
    return;
  }

  // even if etf related types are profile types, but these data should only be stored in td's public.db, so this
  // place should not filter Basket and Basket Instrument
  if (event->msg_type() != Instrument::tag) {
    feed_state_data(event, feed_bank_);
  }

  // only etf related data will be stored by cached, these data should be only store in td public.db, for CachedReset
  if (event->msg_type() != Basket::tag and event->msg_type() != BasketInstrument::tag) {
    feed_profile_data(event, profile_bank_);
  }
}

void cached::switch_feed_storage(bool pause) { storage_pause_ = pause; }

} // namespace kungfu::yijinjing::cache