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
using kungfu::yijinjing::nanomsg::nanomsg_json;

namespace kungfu::wingchun::op {

BacktestContext::BacktestContext(apprentice &app, const rx::connectable_observable<event_ptr> &events)
    : Context(app, events), broker_client_(app) {
  KUNGFU_SETUP_LOGGER(app_.get_home(), app_.get_home()->name);
}

void BacktestContext::on_start() {
  broker_client_.on_start(events_);
  events_ | $$(on_timer_check());
}

bool BacktestContext::is_started() const { return true; }

const std::string BacktestContext::get_config() const {
  // todo figure out how to deal with configure from sqlite.
  return "{}";
}

int64_t BacktestContext::now() const { return app_.now(); }

void BacktestContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {
  pre_timer_callbacks_.emplace(nanotime, callback);
}

void BacktestContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
  auto timer_callback = [this, callback, duration](event_ptr event) {
    callback(event);
    this->add_time_interval(duration, callback);
  };
  pre_timer_callbacks_.emplace(now() + duration, timer_callback);
}

void BacktestContext::on_timer_check() {
  if (not pre_timer_callbacks_.empty()) {
    timer_callbacks_.insert(pre_timer_callbacks_.begin(), pre_timer_callbacks_.end());
    pre_timer_callbacks_.clear();
  }
  auto it = timer_callbacks_.begin();
  auto now_time = now();
  while (it != timer_callbacks_.end()) {
    if (it->first <= now_time) {
      nlohmann::json time_event;
      time_event["msg_type"] = Time::tag;
      time_event["gen_time"] = now_time;
      time_event["trigger_time"] = now_time;
      time_event["source"] = app_.get_home_uid();
      time_event["dest"] = app_.get_home_uid();
      time_event["data"] = nlohmann::json::object();
      it->second(std::make_shared<nanomsg_json>(time_event.dump()));
      it = timer_callbacks_.erase(it);
    } else {
      return;
    }
  }
}

void BacktestContext::subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                                const std::string &exchange_ids) {
  auto md_location = find_md_location(source);
  if (md_location->locator->list_page_id(md_location, location::PUBLIC).empty()) {
    throw wingchun_error(fmt::format("md public journal {} not exists", md_location->uname));
  }
  SPDLOG_INFO("subscribe source={} in: {}", source, md_location->uname);
  add_location(app_, md_location);
  app_.get_reader()->join(md_location, location::PUBLIC, std::max(app_.get_begin_time(), app_.now()));
  for (const auto &instrument_id : instrument_ids) {
    broker_client_.subscribe(md_location, exchange_ids, instrument_id);
  }
}

void BacktestContext::subscribe_all(const std::string &source, uint8_t market_type, uint64_t instrument_type,
                                    uint64_t data_type) {
  auto md_location = find_md_location(source);
  if (md_location->locator->list_page_id(md_location, location::PUBLIC).empty()) {
    throw wingchun_error(fmt::format("md public journal {} not exists", md_location->uname));
  }
  SPDLOG_INFO("subscribe source={} in: {}", source, md_location->uname);
  add_location(app_, md_location);
  app_.get_reader()->join(md_location, location::PUBLIC, std::max(app_.get_begin_time(), app_.now()));
  broker_client_.subscribe_all(find_md_location(source), market_type, instrument_type, data_type);
}

void BacktestContext::subscribe_operator(const std::string &group, const std::string &name) {
  auto op_location = find_op_location(group, name);
  if (op_location->locator->list_page_id(op_location, location::PUBLIC).empty()) {
    throw wingchun_error(fmt::format("op public journal {} not exists", op_location->uname));
  }
  SPDLOG_INFO("subscribe op={}/{} in: {}", group, name, op_location->uname);
  add_location(app_, op_location);
  app_.get_reader()->join(op_location, location::PUBLIC, std::max(app_.get_begin_time(), app_.now()));
  broker_client_.enroll_operator(op_location);
}

void BacktestContext::publish_synthetic_data(const std::string &key, const std::string &value) {
  auto writer = app_.get_writer(location::PUBLIC);
  auto current_time = now();
  SyntheticData synthetic_data;
  synthetic_data.update_time = current_time;
  synthetic_data.key = key;
  synthetic_data.value = value;
  writer->write(current_time, synthetic_data);
}

broker::Client &BacktestContext::get_broker_client() { return broker_client_; }

location_ptr BacktestContext::find_md_location(const std::string &source) {
  uint32_t cache_uid = hash_backtest_cache(source, app_.get_begin_time(), app_.get_end_time());
  auto cache_location =
      location::make_shared(mode::BACKTEST, category::MD, source, fmt::format("{:08x}", cache_uid), app_.get_locator());
  return cache_location;
}

location_ptr BacktestContext::find_op_location(const std::string &group, const std::string &name) {
  uint32_t cache_uid = hash_backtest_cache(name, app_.get_begin_time(), app_.get_end_time());
  auto cache_location = location::make_shared(mode::BACKTEST, category::OPERATOR, group,
                                              fmt::format("{:08x}", cache_uid), app_.get_locator());
  return cache_location;
}

void BacktestContext::req_deregister() {}

void BacktestContext::update_operator_state(OperatorStateUpdate &state_update) {}

yijinjing::data::location_ptr BacktestContext::get_location(uint32_t location_uid) {
  return app_.get_location(location_uid);
}

uint32_t BacktestContext::get_home_uid() const { return app_.get_home_uid(); }
} // namespace kungfu::wingchun::op
