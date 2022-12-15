// SPDX-License-Identifier: Apache-2.0


#include <fmt/format.h>

#include <kungfu/wingchun/operator/runtime.h>
#include <kungfu/yijinjing/log.h>
#include <kungfu/yijinjing/time.h>

using namespace kungfu::yijinjing::practice;
using namespace kungfu::rx;
using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::util;

namespace kungfu::wingchun::op {

RuntimeContext::RuntimeContext(apprentice &app, const rx::connectable_observable<event_ptr> &events)
    : app_(app), events_(events), broker_client_(app_) {
  log::copy_log_settings(app_.get_home(), app_.get_home()->name);
}

void RuntimeContext::on_start() {

}

int64_t RuntimeContext::now() const { return app_.now(); }

void RuntimeContext::add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) {
}

void RuntimeContext::add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) {
}



void RuntimeContext::subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                               const std::string &exchange_ids) {

}

void RuntimeContext::subscribe_all(const std::string &source, uint8_t market_type, uint64_t instrument_type,
                                   uint64_t data_type) {
}

void RuntimeContext::subscribe_operator(const std::string &source, const std::vector<std::string> &keys) {

}


const location_map &RuntimeContext::list_md() const { }


int64_t RuntimeContext::get_trading_day() const {  }

broker::Client &RuntimeContext::get_broker_client() { }





const location_ptr &RuntimeContext::find_md_location(const std::string &source) {

}



void RuntimeContext::req_deregister() { app_.request_deregister(); }

// void RuntimeContext::update_operator_state(StrategyStateUpdate &state_update) {
//   auto writer = app_.get_writer(location::PUBLIC);
//   state_update.update_time = now();
//   writer->write(state_update.update_time, state_update);
// }

} // namespace kungfu::wingchun::op
