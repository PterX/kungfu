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
using namespace kungfu::wingchun::tool;
using kungfu::yijinjing::nanomsg::nanomsg_json;

namespace kungfu::wingchun::op {

BacktestContext::BacktestContext(apprentice &app, const rx::connectable_observable<event_ptr> &events,
                                 SliceIndexer_ptr from_indexer, SliceIndexer_ptr to_indexer, Report_ptr report)
    : Context(app, events), broker_client_(app), from_indexer_(std::move(from_indexer)),
      slice_tool_(std::make_shared<SliceTool>(category::OPERATOR, app.get_home()->group, app.get_home()->name,
                                              std::move(to_indexer))),
      report_(std::move(report)) {
  KUNGFU_SETUP_LOGGER(app_.get_home(), app_.get_home()->name);
}

void BacktestContext::on_start() {
  broker_client_.on_start(events_);
  events_ | is_own<Quote>(get_broker_client()) | $$(report_->on_quote(event->data<Quote>()););
  events_ | is_own<Entrust>(get_broker_client()) | $$(report_->on_entrust(event->data<Entrust>()););
  events_ | is_own<Transaction>(get_broker_client()) | $$(report_->on_transaction(event->data<Transaction>()););
  events_ | is_own<Tree>(get_broker_client()) | $$(report_->on_tree(event->data<Tree>()););
  events_ | is(SyntheticData::tag) | $$(report_->on_read_synthetic_data(event->data<SyntheticData>()));
  events_ | $$(on_timer_check(); lease_expired_check(););
}

bool BacktestContext::is_started() const { return true; }

const std::string BacktestContext::get_config() const {
  // todo figure out how to deal with configure from sqlite.
  return "{}";
}

int64_t BacktestContext::now() const { return app_.now(); }

int32_t BacktestContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {
  const int32_t timer_id = timer_usage_count_++;
  pre_timer_callbacks_.emplace(nanotime, TimerTask{timer_id, callback});
  return timer_id;
}

int32_t BacktestContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
  const int32_t timer_id = timer_usage_count_++;
  return add_timer_interval_helper(duration, timer_id, callback);
}

int32_t BacktestContext::add_timer_interval_helper(int64_t duration, int32_t timer_id,
                                                   const std::function<void(event_ptr)> &callback) {
  auto timer_callback = [this, callback, duration, timer_id](event_ptr event) {
    callback(event);
    this->add_timer_interval_helper(duration, timer_id, callback);
  };
  pre_timer_callbacks_.emplace(now() + duration, TimerTask{timer_id, timer_callback});
  return timer_id;
}

void BacktestContext::clear_timer(int32_t timer_id) {
  std::erase_if(pre_timer_callbacks_,
                [timer_id](const auto &timer_task) { return timer_task.second.timer_id == timer_id; });
  std::erase_if(timer_callbacks_,
                [timer_id](const auto &timer_task) { return timer_task.second.timer_id == timer_id; });
}

void BacktestContext::on_timer_check() {
  if (not pre_timer_callbacks_.empty()) {
    timer_callbacks_.insert(pre_timer_callbacks_.begin(), pre_timer_callbacks_.end());
    pre_timer_callbacks_.clear();
  }
  int64_t now_time = now();
  for (auto it = timer_callbacks_.begin(); it != timer_callbacks_.end();) {
    if (it->first <= now_time) {
      nlohmann::json time_event;
      time_event["msg_type"] = Time::tag;
      time_event["gen_time"] = now_time;
      time_event["trigger_time"] = now_time;
      time_event["source"] = app_.get_home_uid();
      time_event["dest"] = app_.get_home_uid();
      time_event["data"] = nlohmann::json::object();
      it->second.call_back(std::make_shared<nanomsg_json>(time_event.dump()));
      it = timer_callbacks_.erase(it);
    } else {
      return;
    }
  }
}

void BacktestContext::lease_expired_check() {
  int64_t now_time = now();
  for (auto it = lease_locations_.begin(); it != lease_locations_.end();) {
    if (it->first < now_time) {
      for (const auto &expired_location : it->second) {
        SPDLOG_TRACE("sliced location expired, locator={}, location={} disjoining.",
                     expired_location->locator->get_root(), expired_location->uname);
        app_.get_reader()->disjoin(expired_location, location::PUBLIC);
      }
      it = lease_locations_.erase(it);
    } else {
      break;
    }
  }
}

void BacktestContext::subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                                const std::string &exchange_id) {
  for (auto data_type : {Quote::tag, Tree::tag, Entrust::tag, Transaction::tag}) {
    for (const auto &instrument_id : instrument_ids) {
      int64_t slice_begin_time = app_.now();
      int64_t slice_end_time{INT64_MAX};
      do {
        auto md_location = from_indexer_->find_md_slice_location(slice_begin_time, source, source, instrument_id,
                                                                 exchange_id, data_type);
        slice_end_time = from_indexer_->get_md_slice_end_time(slice_begin_time, source, source, instrument_id,
                                                              exchange_id, data_type);
        if (md_location->locator->list_page_id(md_location, location::PUBLIC).empty()) {
          SPDLOG_WARN("md public journal in locator={}, location={} not exists", md_location->locator->get_root(),
                      md_location->uname);
          continue;
        }
        SPDLOG_TRACE("subscribed md public locator={}, location={}", md_location->locator->get_root(),
                     md_location->uname);
        add_location(app_, md_location);
        app_.get_reader()->join(md_location, location::PUBLIC, slice_begin_time);
        broker_client_.subscribe(md_location, exchange_id, instrument_id);
        lease_locations_[slice_end_time].push_back(std::move(md_location));
      } while ((slice_begin_time = 1 + slice_end_time) < app_.get_end_time());
    }
  }
}

void BacktestContext::subscribe_all(const std::string &source, uint8_t market_type, uint64_t instrument_type,
                                    uint64_t data_type) {
  throw std::runtime_error("BacktestContext::subscribe_all not implemented");
}

void BacktestContext::subscribe_operator(const std::string &group, const std::string &name) {
  int64_t slice_begin_time = app_.now();
  int64_t slice_end_time{INT64_MAX};
  do {
    auto op_location = from_indexer_->find_operator_slice_location(slice_begin_time, group, name);
    slice_end_time = from_indexer_->get_operator_slice_end_time(slice_begin_time, group, name);
    if (op_location->locator->list_page_id(op_location, location::PUBLIC).empty()) {
      SPDLOG_WARN("operator public journal in locator={}, location={} not exists", op_location->locator->get_root(),
                  op_location->uname);
      continue;
    }
    SPDLOG_TRACE("subscribed operator public locator={}, location={}", op_location->locator->get_root(),
                 op_location->uname);
    add_location(app_, op_location);
    app_.get_reader()->join(op_location, location::PUBLIC, slice_begin_time);
    broker_client_.enroll_operator(op_location);
    lease_locations_[slice_end_time].push_back(std::move(op_location));
  } while ((slice_begin_time = 1 + slice_end_time) < app_.get_end_time());
}

void BacktestContext::publish_synthetic_data(const std::string &key, const std::string &value) {
  auto current_time = now();
  SyntheticData synthetic_data;
  synthetic_data.update_time = current_time;
  synthetic_data.key = key;
  synthetic_data.value = value;
  slice_tool_->write_at(current_time, current_time, location::PUBLIC, synthetic_data);
}

broker::Client &BacktestContext::get_broker_client() { return broker_client_; }

void BacktestContext::req_deregister() {}

void BacktestContext::update_operator_state(OperatorStateUpdate &state_update) {}

yijinjing::data::location_ptr BacktestContext::get_location(uint32_t location_uid) {
  return app_.get_location(location_uid);
}

uint32_t BacktestContext::get_home_uid() const { return app_.get_home_uid(); }
} // namespace kungfu::wingchun::op