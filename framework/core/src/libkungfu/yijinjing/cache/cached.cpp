// SPDX-License-Identifier: Apache-2.0

#include <kungfu/common.h>
#include <kungfu/longfist/longfist.h>
#include <kungfu/yijinjing/cache/cached.h>
#include <kungfu/yijinjing/time.h>

using namespace kungfu::rx;
using namespace kungfu::longfist;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::cache;

#define DEFAULT_STORE_VOLUME_BY_INTERVAL 100
#define LOW_LATENCY_STORE_VOLUME_BY_INTERVAL 10

namespace kungfu::yijinjing::cache {

cached::cached(locator_ptr locator) : profile_(locator) {
  profile_.setup();
  profile_get_all(profile_, profile_bank_);
}

cached::~cached() {}

void cached::restore(const location_ptr &location, journal::writer_ptr &writer) {
  if (app_cache_shift_.find(location->uid) == app_cache_shift_.end()) {
    make_cache_shift(location);
  }

  try {
    app_cache_shift_.at(location->uid) >> writer;
  } catch (const std::exception &ex) {
    SPDLOG_ERROR("failed to write cache {} {} {}", location->uid, location->uname, ex.what());
  }

  try {
    profile_get_all(profile_, profile_bank_);
    profile_bank_ >> writer;
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
  make_cache_shift(location);
  app_cache_shift_.at(location->uid).ensure_storage(dest);
}

void cached::cache_reset(const CacheReset &cache_reset, uint32_t source_id, uint32_t dest_id) {
  auto msg_type = cache_reset.msg_type;
  boost::hana::for_each(StateDataTypes, [&](auto it) {
    using DataType = typename decltype(+boost::hana::second(it))::type;
    if (DataType::tag == msg_type) {
      if (app_cache_shift_.find(source_id) != app_cache_shift_.end()) {
        app_cache_shift_[source_id] -= typed_event_ptr<DataType>(event);
      }
      if (app_cache_shift_.find(dest_id) != app_cache_shift_.end()) {
        app_cache_shift_[dest_id] /= typed_event_ptr<DataType>(event);
      }
    }
  });
}

void cached::feed(const event_ptr &event) {
  if (event->msg_type() != Instrument::tag and get_location(event->source())->category == category::MD) {
    return;
  }
  feed_state_data(event, feed_bank_);
  feed_profile_data(event, profile_bank_);
}

// void cached::on_react() {
//   events_ | is(Location::tag) | $$(on_location(event));
//   events_ | is(Register::tag) | $$(register_triggger_clear_cache_shift(event->data<Register>()));

// void cached::on_start() {
//   events_ | is(Channel::tag) | $$(inspect_channel(event->gen_time(), event->data<Channel>()));
//   events_ | is(CacheReset::tag) | $$(on_cache_reset(event));
//   events_ | instanceof <journal::frame>() | filter([&](const event_ptr &event) {
//                          auto source_id = event->source();
//                          return source_id != master_home_location_->uid and source_id != master_cmd_location_->uid;
//                        }) | $$(feed(event));
// }

// void cached::on_frame() {}

// void cached::on_active() {
//   handle_cached_feeds(store_volume_every_loop_);
//   handle_profile_feeds(store_volume_every_loop_);
// }

// void cached::on_notify() {
//   SPDLOG_TRACE("cached::on_notify");
//   handle_cached_feeds(LOW_LATENCY_STORE_VOLUME_BY_INTERVAL);
// }

// void cached::handle_cached_feeds(int store_volume_every_loop) {
//   int stored_controller = 0;
//   boost::hana::for_each(StateDataTypes, [&](auto it) {
//     using DataType = typename decltype(+boost::hana::second(it))::type;
//     auto hana_type = boost::hana::type_c<DataType>;

//     using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
//     auto &feed_map = const_cast<FeedMap &>(feed_bank_[hana_type]);

//     if (feed_map.size() != 0) {
//       auto iter = feed_map.begin();
//       while (iter != feed_map.end() and stored_controller <= store_volume_every_loop) {
//         auto &s = iter->second;
//         auto source_id = s.source;
//         auto dest_id = s.dest;
//         if (app_cache_shift_.find(source_id) != app_cache_shift_.end()) {
//           try {
//             app_cache_shift_.at(source_id) << s;
//             SPDLOG_TRACE("cache [feed] source {} dest {} {} data {}", get_location_uname(source_id),
//                          get_location_uname(dest_id), DataType::type_name.c_str(), s.data.to_string());
//           } catch (const std::exception &e) {
//             SPDLOG_ERROR("Unexpected exception by handle_cached_feeds {}", e.what());
//             stored_controller++;
//             break;
//           }

//           iter = feed_map.erase(iter);
//           stored_controller++;
//         } else {
//           iter++;
//         }
//       }
//     }
//   });
// }

// void cached::handle_profile_feeds(int store_volume_every_loop) {
//   int stored_controller = 0;
//   boost::hana::for_each(ProfileDataTypes, [&](auto it) {
//     using DataType = typename decltype(+boost::hana::second(it))::type;
//     auto hana_type = boost::hana::type_c<DataType>;

//     using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
//     auto &feed_map = const_cast<FeedMap &>(profile_bank_[hana_type]);

//     if (feed_map.size() != 0) {
//       auto iter = feed_map.begin();
//       while (iter != feed_map.end() and stored_controller <= store_volume_every_loop) {
//         const auto &s = iter->second;
//         try {
//           profile_ << s;
//           SPDLOG_TRACE("cache [profile] {} data {}", DataType::type_name.c_str(), s.data.to_string());
//         } catch (const std::exception &e) {
//           SPDLOG_ERROR("Unexpected exception by handle_profile_feeds {}", e.what());
//           stored_controller++;
//           break;
//         }

//         iter = feed_map.erase(iter);
//         stored_controller++;
//       }
//     }
//   });
// }

// void cached::on_location(const event_ptr &event) { profile_bank_ << typed_event_ptr<Location>(event); }

// void cached::inspect_channel(int64_t trigger_time, const Channel &channel) {
//   if (channel.source_id != get_live_home_uid() and channel.dest_id != get_live_home_uid()) {
//     reader_->join(get_location(channel.source_id), channel.dest_id, trigger_time);
//     make_cache_shift(channel.source_id, channel.dest_id);
//   }
// }

// void cached::register_trigger_listen_public(int64_t gen_time, const Register &register_data) {
//   auto app_uid = register_data.location_uid;
//   auto app_location = get_location(app_uid);

//   if (app_location->category != category::TD) {
//     return;
//   }

//   reader_->join(app_location, location::PUBLIC, gen_time);
//   make_cache_shift(app_uid, location::PUBLIC);
//   reader_->join(app_location, location::SYNC, gen_time);
//   make_cache_shift(app_uid, location::SYNC);
//   SPDLOG_INFO("resume {} connection from {}", get_location_uname(app_uid), time::strftime(gen_time));
// }

// void cached::feed(const event_ptr &event) {
//   if (event->msg_type() != Instrument::tag and get_location(event->source())->category == category::MD) {
//     return;
//   }
//   feed_state_data(event, feed_bank_);
//   feed_profile_data(event, profile_bank_);
// }

} // namespace kungfu::yijinjing::cache