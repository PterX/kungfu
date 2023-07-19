#include <kungfu/wingchun/broker/algoorderengine.h>
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

namespace kungfu::wingchun::broker::algoorder {

AlgoOrderEngine::AlgoOrderEngine(BrokerVendor &vendor) : vendor_(vendor) {}

void AlgoOrderEngine::on_start(const rx::connectable_observable<event_ptr> &events) {
  events | is(AlgoOrderInput::tag) | $$(update_algo_order(event, event->data<AlgoOrderInput>()));
  events | is(AlgoOrderAction::tag) | $$(cancel_algo_order(event, event->data<AlgoOrderAction>()));
}

BrokerService_ptr AlgoOrderEngine::get_service() { return vendor_.get_service(); }

void AlgoOrderEngine::update_algo_order(const event_ptr &event,
                                        const longfist::types::AlgoOrderInput &algo_order_input) {
  auto algo_order = dynamic_cast<Trader &>(*get_service()).insert_algo_order(algo_order_input);
  vendor_.get_writer(event->source())->write(vendor_.now(), algo_order);

  if (algo_order_input.is_local) {
    state<AlgoOrder> algo_order_state(event->dest(), event->source(), event->gen_time(), algo_order);
    local_algo_orders_.insert_or_assign(algo_order_input.order_id, algo_order_state);
  }
}

void AlgoOrderEngine::update_algo_order(const longfist::types::Order &order) {
  if (order.parent_id == UINT64_ZERO) {
    return;
  }

  if (local_algo_orders_.find(order.parent_id) == local_algo_orders_.end()) {
    return;
  }

  try_update_sub_orders(order);

  auto &target_algo_order_state = local_algo_orders_.at(order.parent_id);
  auto &target_algo_order = target_algo_order_state.data;
  auto dest = target_algo_order_state.dest;

  auto volume_traded = order.volume - order.volume_left;
  target_algo_order.volume_left -= volume_traded;

  auto all_order_finished = check_if_all_order_finished(order.parent_id);
  auto has_traded = target_algo_order.volume_left != target_algo_order.volume;

  if (target_algo_order.volume_left == 0) {
    target_algo_order.status = OrderStatus::Filled;
  } else if (has_traded && all_order_finished) {
    target_algo_order.status = OrderStatus::PartialFilledNotActive;
  } else if (has_traded && !all_order_finished) {
    target_algo_order.status = OrderStatus::PartialFilledActive;
  }

  vendor_.get_writer(dest)->write(time::now_in_nano(), target_algo_order);
}

void AlgoOrderEngine::try_update_sub_orders(const longfist::types::Order &order) {
  if (local_sub_orders_.find(order.parent_id) == local_sub_orders_.end()) {
    OrderMap orders;
    local_sub_orders_.emplace(order.parent_id, orders);
  }

  auto &orders = local_sub_orders_.at(order.parent_id);
  orders.insert_or_assign(order.order_id, order);
}

bool AlgoOrderEngine::check_if_all_order_finished(int64_t algo_order_id) {
  if (local_sub_orders_.find(algo_order_id) == local_sub_orders_.end()) {
    SPDLOG_ERROR("check_if_all_order_finished no {} in local_sub_orders_", algo_order_id);
    return false;
  }
  auto &orders = local_sub_orders_.at(algo_order_id);
  return is_all_order_finished(orders);
}

void AlgoOrderEngine::cancel_algo_order(const event_ptr &event,
                                        const longfist::types::AlgoOrderAction &algo_order_action) {
  // no algo order action resolution for local algo order;
  if (local_algo_orders_.find(algo_order_action.order_id) != local_algo_orders_.end()) {
    return;
  }

  dynamic_cast<Trader &>(*get_service()).cancel_algo_order(event);
}

} // namespace kungfu::wingchun::broker::algoorder
