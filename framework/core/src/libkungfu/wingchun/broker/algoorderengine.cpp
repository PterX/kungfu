#include <kungfu/wingchun/broker/algoorderengine.h>

using namespace kungfu::rx;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::broker;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::util;

namespace kungfu::wingchun::broker {
    namespace algoorder {

AlgoOrderEngine::AlgoOrderEngine(TraderVendor &vendor) : vendor_(vendor) {}

void AlgoOrderEngine::on_start(const rx::connectable_observable<event_ptr> &events) {
    events | is(AlgoOrderInput::tag) | $$(update_algo_order(event->source(), event->data<AlgoOrderInput>()));
    events | is(Order::tag) | $$(update_algo_order(event->trigger_time(), event->data<Order>()));
}

BrokerService_ptr AlgoOrderEngine::get_service() {
    return vendor_.get_service();
}

void AlgoOrderEngine::update_algo_order(uint32_t source, const longfist::types::AlgoOrderInput &algo_order_input) {
    auto algo_order = dynamic_cast<Trader &>(*get_service()).insert_algo_order(algo_order_input);
    vendor_.get_writer(source)->write(time::now_in_nano(), algo_order);
}

void AlgoOrderEngine::update_algo_order(int64_t trigger_time, const longfist::types::Order& order) {}           


  } //namespace algoorder

} // namespace kungfu::wingchun::broker
