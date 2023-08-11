// SPDX-License-Identifier: Apache-2.0

#include <fmt/format.h>
#include <kungfu/wingchun/operator/replay.h>
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
using namespace kungfu::yijinjing::journal;

namespace kungfu::wingchun::op {

ReplayContext::ReplayContext(apprentice &app, const rx::connectable_observable<event_ptr> &events)
    : Context(app, events), broker_client_(app),
      reader_for_write_(std::make_shared<reader>(true, false, std::make_shared<bus>(false))) {
  log::copy_log_settings(app_.get_home(), app_.get_home()->name);
  auto live_home = app_.get_live_home();
  for (auto dest_id : live_home->locator->list_location_dest(live_home)) {
    if (page::check_page_existed(live_home, dest_id)) {
      reader_for_write_->join(live_home, dest_id, app_.get_begin_time());
    } else {
      SPDLOG_WARN("page not existed, source_location: {}, dest: {}", live_home->uname, (uint32_t)dest_id);
    }
  }
}

void ReplayContext::on_start() {
  broker_client_.on_start(events_);
  events_ | $$(on_timer_check());
}

void ReplayContext::prepare(const event_ptr &event) {
  if (not broker_client_.enrolled_md_ready() or not broker_client_.enrolled_operator_ready()) {
    return;
  }
  started_ = true;
}

bool ReplayContext::is_started() const { return started_; }

int64_t ReplayContext::now() const { return app_.now(); }

uint32_t ReplayContext::get_home_uid() const { return app_.get_home_uid(); }

uint32_t ReplayContext::get_live_home_uid() const { return app_.get_live_home_uid(); }

const std::string ReplayContext::get_config() const {
  auto &config_map = app_.get_state_bank()[boost::hana::type_c<Config>];
  if (config_map.find(app_.get_live_home_uid()) == config_map.end()) {
    return "{}";
  }
  auto &config_obj = config_map.at(app_.get_live_home_uid());
  return config_obj.data.value;
}

void ReplayContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {
  app_.add_timer(nanotime, callback);
}

void ReplayContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
  app_.add_time_interval(duration, callback);
}

void ReplayContext::subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                              const std::string &exchange_id) {
  auto md_location = broker_client_.find_md_location(source, app_.get_home());
  for (const auto &instrument_id : instrument_ids) {
    broker_client_.subscribe(md_location, exchange_id, instrument_id);
  }
}

void ReplayContext::subscribe_all(const std::string &source, uint8_t market_type, uint64_t instrument_type,
                                  uint64_t data_type) {
  auto md_location = broker_client_.find_md_location(source, app_.get_home());
  broker_client_.subscribe_all(md_location, market_type, instrument_type, data_type);
}

void ReplayContext::subscribe_operator(const std::string &group, const std::string &name) {
  uint32_t hashed_op = hash_operator(group, name);

  auto home = app_.get_live_home();
  auto operator_location = location::make_shared(mode::LIVE, category::OPERATOR, group, name, home->locator);
  if (not app_.has_location(operator_location->uid)) {
    throw wingchun_error(fmt::format("invalid operator {}_{}", group, name));
  }

  broker_client_.enroll_operator(operator_location);
}

void ReplayContext::publish_synthetic_data(const std::string &key, const std::string &value) {
  auto frame = read_next(SyntheticData::tag);
  auto &synthetic_data = frame->data<SyntheticData>();
  SPDLOG_DEBUG("publish_synthetic_data key {}, value {}, result {}", key, value, synthetic_data.to_string());
}

void ReplayContext::req_deregister() {
  SPDLOG_WARN("req_deregister");
  app_.request_deregister();
}

void ReplayContext::update_operator_state(OperatorStateUpdate &state_upate) {}

broker::Client &ReplayContext::get_broker_client() { return broker_client_; }

yijinjing::data::location_ptr ReplayContext::get_location(uint32_t location_uid) {
  return app_.get_location(location_uid);
}

frame_ptr ReplayContext::read_next(uint32_t msg_type) {
  while (reader_for_write_->data_available()) {
    auto frame = reader_for_write_->current_frame();
    if (frame->msg_type() == msg_type && frame->gen_time() >= now()) {
      break;
    }
    reader_for_write_->next();
  }

  if (not reader_for_write_->data_available()) {
    req_deregister();
    SPDLOG_ERROR("no more data available");
    throw wingchun_error("no more data available");
  }

  return reader_for_write_->current_frame();
}

void ReplayContext::on_timer_check() {
  if (now() >= app_.get_end_time()) {
    SPDLOG_WARN("end time limit exceeded");
    req_deregister();
  }
}

} // namespace kungfu::wingchun::op