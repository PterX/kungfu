// SPDX-License-Identifier: Apache-2.0
#include <kungfu/wingchun/operator/runner.h>

using namespace kungfu::rx;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun::broker;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::practice;

namespace kungfu::wingchun::op {
Runner::Runner(locator_ptr locator, const std::string &group, const std::string &name, mode m, bool low_latency)
    : apprentice(location::make_shared(m, category::MD, group, name, std::move(locator)), low_latency),
      started_(m == mode::BACKTEST) {}

RuntimeContext_ptr Runner::get_context() const {  }

RuntimeContext_ptr Runner::make_context() {  }

void Runner::add_operator(const Operator_ptr &op) {  }

void Runner::on_exit() {}

void Runner::on_trading_day(const event_ptr &event, int64_t daytime) {
}

void Runner::on_react() {
	
}

void Runner::inspect_channel(const event_ptr &event) {

}

void Runner::on_start() {

}

void Runner::on_active() {

}

void Runner::pre_start() {  }

void Runner::post_start() {

}

void Runner::pre_stop() { }

void Runner::post_stop() { }

void Runner::prepare(const event_ptr &event) {

}



} // namespace kungfu::wingchun::op
