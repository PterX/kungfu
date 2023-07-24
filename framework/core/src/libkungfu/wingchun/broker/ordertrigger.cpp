#include <kungfu/wingchun/broker/trader.h>

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

void OrderTriggerService::on_order_trigger_input(const event_ptr &event) { get_service().insert_order_trigger(event); }

void OrderTriggerService::on_order_trigger_action(const event_ptr &event) { get_service().cancel_order_trigger(event); }

void OrderTriggerService::on_order_trigger(uint32_t source, uint32_t dest, int64_t gen_time,
                                           const longfist::types::OrderTrigger &order_trigger) {
  state<OrderTrigger> order_trigger_state(source, dest, gen_time, order_trigger);
  triggers_.insert_or_assign(order_trigger.trigger_id, order_trigger_state);
}

void OrderTriggerService::clean_order_triggers(bool bypass_recover) {
  std::for_each(triggers_.begin(), triggers_.end(), [&](auto &pair) {
    OrderTrigger &trigger = pair.second.data;
    if (not is_final_status(trigger.status) and (bypass_recover or trigger.external_trigger_id.to_string().empty())) {
      trigger.status = OrderStatus::Lost;
      trigger.update_time = time::now_in_nano();
      if (vendor_.has_writer(pair.second.dest)) {
        vendor_.write_to(vendor_.now(), trigger, pair.second.dest);
      }
    }
  });
}

void OrderTriggerService::clean_order_triggers(uint32_t source,
                                               const longfist::types::OrderTriggerInput &order_trigger_input,
                                               bool bypass_recover) {
  if (triggers_.find(order_trigger_input.trigger_id) != triggers_.end()) {
    return;
  }
  if (not vendor_.has_writer(source)) {
    return;
  }

  auto writer = vendor_.get_writer(source);
  OrderTrigger &trigger = writer->open_data<OrderTrigger>();
  order_trigger_from_input(order_trigger_input, trigger);
  trigger.status = OrderStatus::Lost;
  trigger.update_time = time::now_in_nano();
  writer->close_data();
}

const OrderTriggerMap &OrderTriggerService::get_order_triggers() const { return triggers_; }

bool OrderTriggerService::has_order_trigger(uint64_t trigger_id) const {
  return triggers_.find(trigger_id) != triggers_.end();
}

kungfu::state<longfist::types::OrderTrigger> &OrderTriggerService::get_order_trigger(uint64_t trigger_id) {
  return triggers_.at(trigger_id);
}

} // namespace kungfu::wingchun::broker