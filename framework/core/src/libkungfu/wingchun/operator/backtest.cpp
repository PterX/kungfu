// SPDX-License-Identifier: Apache-2.0
#include <kungfu/wingchun/operator/backtest.h>

using namespace kungfu::yijinjing::practice;
using namespace kungfu::rx;
using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::util;

namespace kungfu::wingchun::op {

BacktestContext::BacktestContext(apprentice &app, const rx::connectable_observable<event_ptr> &events)
    : Context(app, events), broker_client_(app) {
  log::copy_log_settings(app_.get_home(), app_.get_home()->name);
}

void BacktestContext::on_start() { broker_client_.on_start(events_); }

bool BacktestContext::is_started() const { return true; }

const std::string BacktestContext::get_config() const {}

int64_t BacktestContext::now() const { return app_.now(); }

void BacktestContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {}

void BacktestContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
  app_.add_time_interval(duration, callback);
}

void BacktestContext::subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                                const std::string &exchange_ids) {}

void BacktestContext::subscribe_all(const std::string &source, uint8_t market_type, uint64_t instrument_type,
                                    uint64_t data_type) {
  //   broker_client_.subscribe_all(find_md_location(source), market_type, instrument_type, data_type);
}

void BacktestContext::subscribe_operator(const std::string &group, const std::string &name) {}

void BacktestContext::publish_synthetic_data(const std::string &key, const std::string &value) {
  //   auto writer = app_.get_writer(location::PUBLIC);
  //   auto current_time = now();
  //   SyntheticData synthetic_data;
  //   synthetic_data.update_time = current_time;
  //   synthetic_data.key = key;
  //   synthetic_data.value = value;
  //   writer->write(current_time, synthetic_data);
}

// const location_map &BacktestContext::list_md() const { return md_locations_; }

// const location_map &BacktestContext::list_op() const { return op_locations_; }

int64_t BacktestContext::get_trading_day() const { return app_.get_trading_day(); }

broker::Client &BacktestContext::get_broker_client() { return broker_client_; }

// const location_ptr &BacktestContext::find_md_location(const std::string &source) {
//   return find_location(source, category::MD, market_data_);
// }

// const location_ptr &
// BacktestContext::find_location(const std::string &source, category c,
//                            std::unordered_map<std::string, yijinjing::data::location_ptr> &locations) {
//   if (locations.find(source) == locations.end()) {
//     auto home_locator = app_.get_locator();
//     auto source_location = location::make_shared(mode::LIVE, c, source, source, home_locator);
//     if (not app_.has_location(source_location->uid)) {
//       throw wingchun_error(fmt::format("invalid {} {}", get_category_name(c), source));
//     }
//     locations.emplace(source, source_location);
//   }
//   return locations.at(source);
// }

void BacktestContext::req_deregister() {}

void BacktestContext::update_operator_state(OperatorStateUpdate &state_update) {}

} // namespace kungfu::wingchun::op
