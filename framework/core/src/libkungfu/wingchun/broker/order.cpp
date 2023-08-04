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

void OrderService::on_order_input(const event_ptr &event) {
  auto &order_input = event->data<OrderInput>();
  auto risk_uid = get_service().get_risk_uid();
  if (risk_uid != 0 and event->initial_source() != risk_uid) {
    auto dest = event->dest();
    if (not vendor_.has_writer(dest)) {
      SPDLOG_ERROR("no writer for {}", vendor_.get_location_uname(dest));
      return;
    }

    auto writer = vendor_.get_writer(dest);
    auto &order = writer->open_data<Order>();
    order_from_input(order_input, order);
    std::string error_msg = "Risk uid not match";
    strncpy(order.error_msg, error_msg.c_str(), ERROR_MSG_LEN);
    order.status = OrderStatus::Error;
    writer->close_data();
    return;
  }

  // try_emplace default insert false to map, means not batch mode
  if (batch_status_.try_emplace(event->source()).first->second) {
    batch_order_inputs_.try_emplace(event->source()).first->second.push_back(order_input);
    return;
  }

  if (block_messages_.find(order_input.block_id) != block_messages_.end()) {
    auto &block_message = block_messages_.at(order_input.block_id);
    get_service().insert_block_order(event, block_message);
    return;
  }

  get_service().insert_order(event);
}

void OrderService::on_order_action(const event_ptr &event) {
  get_service().cancel_order(event);
  const auto &order_action = event->data<OrderAction>();
  state<OrderAction> order_action_state(event->source(), event->dest(), event->gen_time(), order_action);
  order_actions_.insert_or_assign(order_action.order_id, order_action_state);
}

void OrderService::on_order(int64_t gen_time, uint32_t source, uint32_t dest, const Order &order) {
  state<Order> order_state(source, dest, gen_time, order);
  orders_.insert_or_assign(order.order_id, order_state);
}

void OrderService::on_trade(int64_t gen_time, uint32_t source, uint32_t dest, const Trade &trade) {
  state<Trade> trade_state(source, dest, gen_time, trade);
  trades_.insert_or_assign(trade.trade_id, trade_state);
}

void OrderService::on_batch_order_tag(const event_ptr &event) {
  if (event->msg_type() == BatchOrderBegin::tag) {
    batch_status_.insert_or_assign(event->source(), true);
    return;
  }

  if (event->msg_type() == BatchOrderEnd::tag) {
    batch_status_.insert_or_assign(event->source(), false);
    get_service().insert_batch_orders(event, batch_order_inputs_.at(event->source()));
    clear_batch_order_inputs(event->source());
  }
}

void OrderService::on_block_message(const longfist::types::BlockMessage &block_message) {
  if (block_message.block_id == UINT64_ZERO) {
    return;
  }
  block_messages_.insert_or_assign(block_message.block_id, block_message);
}

void OrderService::clear_batch_order_inputs(uint32_t location_uid) { batch_order_inputs_.erase(location_uid); }

void OrderService::clean_orders(bool bypass_recover) {
  std::for_each(orders_.begin(), orders_.end(), [&](auto &pair) {
    Order &order = pair.second.data;
    if (not is_final_status(order.status) and (bypass_recover or order.external_order_id.to_string().empty())) {
      order.status = OrderStatus::Lost;
      order.update_time = time::now_in_nano();
      if (vendor_.has_writer(pair.second.dest)) {
        vendor_.write_to(vendor_.now(), order, pair.second.dest);
      }
    }
  });
}

void OrderService::clean_orders(uint32_t source, const OrderInput &order_input, bool bypass_recover) {
  if (orders_.find(order_input.order_id) != orders_.end()) {
    return;
  }
  if (not vendor_.has_writer(source)) {
    return;
  }

  auto writer = vendor_.get_writer(source);
  Order &order = writer->open_data<Order>();
  order_from_input(order_input, order);
  order.status = OrderStatus::Lost;
  order.update_time = time::now_in_nano();
  writer->close_data();
}

const OrderMap &OrderService::get_orders() const { return orders_; }

bool OrderService::has_order(uint64_t order_id) const { return orders_.find(order_id) != orders_.end(); }

kungfu::state<longfist::types::Order> &OrderService::get_order(uint64_t order_id) { return orders_.at(order_id); }

const OrderActionMap &OrderService::get_order_actions() const { return order_actions_; }

const TradeMap &OrderService::get_trades() const { return trades_; }

bool OrderService::has_order_action(uint64_t action_id) const {
  return order_actions_.find(action_id) != order_actions_.end();
}

kungfu::state<longfist::types::OrderAction> &OrderService::get_order_action(uint64_t action_id) {
  return order_actions_.at(action_id);
}

} // namespace kungfu::wingchun::broker