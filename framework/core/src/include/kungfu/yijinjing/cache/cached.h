// SPDX-License-Identifier: Apache-2.0

#ifndef KUNGFU_CACHED_H
#define KUNGFU_CACHED_H

#include <kungfu/yijinjing/cache/profile.h>
#include <kungfu/yijinjing/cache/runtime.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/log.h>
#include <kungfu/yijinjing/practice/apprentice.h>

namespace kungfu::yijinjing::cache {

using ProfileDataTypesType = decltype(longfist::ProfileDataTypes);
using ProfileStateMapType = decltype(longfist::build_state_map(longfist::ProfileDataTypes));
typedef yijinjing::cache::typed_bank<ProfileDataTypesType, ProfileStateMapType> ProfileStateBank;

class cached : public yijinjing::practice::apprentice {
public:
  explicit cached(yijinjing::data::locator_ptr locator, longfist::enums::mode m, bool low_latency,
                  const std::string &arguments);

protected:
  void on_start() override;

  void on_react() override;

  void on_active() override;

  static constexpr auto profile_get_all = [](auto &profile, auto &receiver) {
    boost::hana::for_each(longfist::ProfileDataTypes, [&](auto it) {
      auto type = boost::hana::second(it);
      using DataType = typename decltype(+type)::type;
      int get_all_count = 0;
      while (get_all_count++ < 10) {
        try {
          for (const auto &data : profile.get_all(DataType{})) {
            auto s = state(0, 0, 0, data);
            receiver << s;
          }
          break;
        } catch (const std::exception &e) {
          SPDLOG_ERROR("Unexpected exception by profile_get_all {}", e.what());
        }
      }
    });
  };

private:
  std::unordered_map<uint32_t, yijinjing::cache::shift> app_cache_shift_ = {};
  yijinjing::cache::bank feed_bank_;
  yijinjing::cache::profile profile_;
  ProfileStateBank profile_bank_ = ProfileStateBank(longfist::ProfileDataTypes);
  bool storage_pause_ = false;

  void on_location(const event_ptr &event);

  void handle_cached_feeds(int store_volume_every_loop);

  void handle_profile_feeds(int store_volume_every_loop);

  void mark_request_cached_done(uint32_t dest_id);

  void inspect_channel(int64_t trigger_time, const longfist::types::Channel &channel);

  void make_cache_shift(uint32_t source_id, uint32_t dest_id);

  void register_triggger_clear_cache_shift(const longfist::types::Register &deregister_data);

  void register_trigger_listen_public(int64_t trigger_time, const longfist::types::Register &register_data);

  void on_cache_reset(const event_ptr &event);

  void ensure_cached_storage(uint32_t source_id, uint32_t dest_id);

  void feed(const event_ptr &event);

  void switch_feed_storage(bool pause);

  template <typename SourceType, typename DestType>
  static constexpr auto transfer_from_bank =
      [](auto datatypes, SourceType &data_source, DestType &data_dest, int32_t limit) {
        auto count = 0;
        boost::hana::for_each(datatypes, [&](auto it) {
          using DataType = typename decltype(+boost::hana::second(it))::type;
          auto hana_type = boost::hana::type_c<DataType>;
          using FeedMap = std::unordered_map<uint64_t, state<DataType>>;
          auto &feed_map = const_cast<FeedMap &>(data_source[hana_type]);
          auto iter = feed_map.begin();
          while (iter != feed_map.end() and count < limit) {
            data_dest << iter->second;
            iter = feed_map.erase(iter);
            count++;
          }
        });
        return count;
      };
};

} // namespace kungfu::yijinjing::cache

#endif // KUNGFU_CACHED_H
