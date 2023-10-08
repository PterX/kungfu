// SPDX-License-Identifier: Apache-2.0

#include <fmt/format.h>

#include <kungfu/wingchun/operator/runtime.h>
#include <kungfu/yijinjing/log.h>
#include <kungfu/yijinjing/time.h>

using namespace kungfu::yijinjing::practice;
using namespace kungfu::rx;
using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::util;

namespace kungfu::wingchun::op {

RuntimeContext::RuntimeContext(apprentice &app, const rx::connectable_observable<event_ptr> &events)
    : app_(app), events_(events), broker_client_(app_),basketorder_engine_(app_) {
  log::copy_log_settings(app_.get_home(), app_.get_home()->name);
}

void RuntimeContext::on_start() { 
  broker_client_.on_start(events_); 
  basketorder_engine_.on_start(events_);
}

const std::string RuntimeContext::get_config() const {
  auto &config_map = app_.get_state_bank()[boost::hana::type_c<Config>];
  if (config_map.find(app_.get_home_uid()) == config_map.end()) {
    return "{}";
  }
  auto &config_obj = config_map.at(app_.get_home_uid());
  return config_obj.data.value;
}

int64_t RuntimeContext::now() const { return app_.now(); }

int32_t RuntimeContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {
  return app_.add_timer(nanotime, callback);
}

int32_t RuntimeContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
  return app_.add_time_interval(duration, callback);
}

void RuntimeContext::clear_timer(int32_t timer_id) { app_.clear_timer(timer_id); }

void RuntimeContext::subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                               const std::string &exchange_ids) {
  auto md_location = find_md_location(source);
  for (const auto &instrument_id : instrument_ids) {
    broker_client_.subscribe(md_location, exchange_ids, instrument_id);
  }
  md_locations_.emplace(md_location->uid, md_location);
}

void RuntimeContext::subscribe_all(const std::string &source, uint8_t market_type, uint64_t instrument_type,
                                   uint64_t data_type) {
  broker_client_.subscribe_all(find_md_location(source), market_type, instrument_type, data_type);
}

void RuntimeContext::subscribe_operator(const std::string &group, const std::string &name) {
  uint32_t hashed_op = hash_operator(group, name);

  if (op_locations_.find(hashed_op) != op_locations_.end()) {
    throw wingchun_error(fmt::format("duplicated operator subscribed {}_{}", group, name));
  }

  auto home = app_.get_home();
  auto operator_location = location::make_shared(mode::LIVE, category::OPERATOR, group, name, home->locator);
  if (home->mode == mode::LIVE and not app_.has_location(operator_location->uid)) {
    throw wingchun_error(fmt::format("invalid operator {}_{}", group, name));
  }

  // op_locations_.emplace(hashed_op, operator_location);
  op_locations_.emplace(operator_location->uid, operator_location);

  broker_client_.enroll_operator(operator_location);
}

void RuntimeContext::publish_synthetic_data(const std::string &key, const std::string &value) {
  auto writer = app_.get_writer(location::PUBLIC);
  auto current_time = now();
  SyntheticData synthetic_data;
  synthetic_data.update_time = current_time;
  synthetic_data.key = key;
  synthetic_data.value = value;
  writer->write(current_time, synthetic_data);
}

const location_map &RuntimeContext::list_md() const { return md_locations_; }

const location_map &RuntimeContext::list_op() const { return op_locations_; }

int64_t RuntimeContext::get_trading_day() const { return app_.get_trading_day(); }

broker::PassiveClient &RuntimeContext::get_broker_client() { return broker_client_; }

void RuntimeContext::check_dependency_state(const event_ptr &event) {
  bool all_dependency_ready = true;
  if (event->msg_type() == BrokerStateUpdate::tag) {
    if (event->data<BrokerStateUpdate>().state == BrokerState::Ready and state_ == OperatorState::Ready)
      return;
    if (event->data<BrokerStateUpdate>().state != BrokerState::Ready and state_ != OperatorState::Ready)
      return;
    all_dependency_ready = broker_client_.enrolled_md_ready();
  }
  if (event->msg_type() == OperatorStateUpdate::tag) {
    if (event->data<OperatorStateUpdate>().state == OperatorState::Ready and state_ == OperatorState::Ready)
      return;
    if (event->data<OperatorStateUpdate>().state != OperatorState::Ready and state_ != OperatorState::Ready)
      return;
    all_dependency_ready = broker_client_.enrolled_operator_ready();
  }
  if (event->msg_type() == Deregister::tag) {
    all_dependency_ready = broker_client_.enrolled_operator_ready() and broker_client_.enrolled_md_ready();
  }
  OperatorStateUpdate state_update;
  if (not all_dependency_ready) {
    state_update.state = OperatorState::DisConnected;
    update_operator_state(state_update);
  } else {
    state_update.state = OperatorState::Ready;
    update_operator_state(state_update);
  }
  SPDLOG_DEBUG("checked dependency, all dependency ready={}, ", all_dependency_ready);
}

const location_ptr &RuntimeContext::find_md_location(const std::string &source) {
  return find_location(source, category::MD, market_data_);
}

const location_ptr &
RuntimeContext::find_location(const std::string &source, category c,
                              std::unordered_map<std::string, yijinjing::data::location_ptr> &locations) {
  if (locations.find(source) == locations.end()) {
    auto home_locator = app_.get_locator();
    auto source_location = location::make_shared(mode::LIVE, c, source, source, home_locator);
    if (not app_.has_location(source_location->uid)) {
      throw wingchun_error(fmt::format("invalid {} {}", get_category_name(c), source));
    }
    locations.emplace(source, source_location);
  }
  return locations.at(source);
}

void RuntimeContext::req_deregister() { app_.request_deregister(); }

void RuntimeContext::update_operator_state(OperatorStateUpdate &state_update) {
  state_ = state_update.state;
  auto writer = app_.get_writer(location::PUBLIC);
  state_update.update_time = now();
  state_update.location_uid = app_.get_home_uid();
  writer->write(state_update.update_time, state_update);
}

basketorder::BasketOrderEngine &RuntimeContext::get_basketorder_engine() { return basketorder_engine_; }

} // namespace kungfu::wingchun::op
