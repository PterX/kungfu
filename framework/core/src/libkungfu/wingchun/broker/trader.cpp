// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2019-06-20.
//

#include <kungfu/common.h>
#include <kungfu/wingchun/broker/trader.h>
#include <kungfu/yijinjing/journal/tracer.h>
#include <kungfu/yijinjing/time.h>

using namespace kungfu::rx;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;

namespace kungfu::wingchun::broker {

bool Trader::insert_algo_order(const event_ptr &event) {
  auto writer = get_writer(event->source());
  auto &algo_order_input = event->data<longfist::types::AlgoOrderInput>();
  auto &algo_order = writer->open_data<AlgoOrder>();
  algo_order_from_input(algo_order_input, algo_order);
  if (not algo_order_input.is_local) {
    algo_order.status = longfist::enums::OrderStatus::Error;
    std::string error_msg = "Algo not supported";
    strcpy(algo_order.error_msg, error_msg.c_str());
  }
  writer->close_data();
  return true;
}

[[maybe_unused]] const std::string &Trader::get_account_id() const { return get_home()->name; }

yijinjing::journal::writer_ptr Trader::get_asset_writer() const {
  return get_writer(sync_asset_ ? location::SYNC : location::PUBLIC);
}

yijinjing::journal::writer_ptr Trader::get_position_writer() const {
  return get_writer(sync_position_ ? location::SYNC : location::PUBLIC);
}

AlgoOrderService &Trader::get_algo_order_service() {
  return dynamic_cast<TraderVendor &>(get_vendor()).get_algo_order_service();
}

const AlgoOrderService &Trader::get_algo_order_service() const {
  return dynamic_cast<TraderVendor &>(get_vendor()).get_algo_order_service();
}

OrderService &Trader::get_order_service() { return dynamic_cast<TraderVendor &>(get_vendor()).get_order_service(); }

const OrderService &Trader::get_order_service() const {
  return dynamic_cast<TraderVendor &>(get_vendor()).get_order_service();
}

OrderTriggerService &Trader::get_order_trigger_service() {
  return dynamic_cast<TraderVendor &>(get_vendor()).get_order_trigger_service();
}

const OrderTriggerService &Trader::get_order_trigger_service() const {
  return dynamic_cast<TraderVendor &>(get_vendor()).get_order_trigger_service();
}

const OrderMap &Trader::get_orders() const { return get_order_service().get_orders(); }

bool Trader::has_order(uint64_t order_id) const { return get_order_service().has_order(order_id); }

state<Order> &Trader::get_order(uint64_t order_id) { return get_order_service().get_order(order_id); }

const OrderActionMap &Trader::get_order_actions() const { return get_order_service().get_order_actions(); }

bool Trader::has_order_action(uint64_t action_id) const { return get_order_service().has_order_action(action_id); }

kungfu::state<longfist::types::OrderAction> &Trader::get_order_action(uint64_t action_id) {
  return get_order_service().get_order_action(action_id);
}

const TradeMap &Trader::get_trades() const { return get_order_service().get_trades(); }

const OrderTriggerMap &Trader::get_order_triggers() const { return get_order_trigger_service().get_order_triggers(); }

bool Trader::has_order_trigger_action(uint64_t action_id) const {
  return get_order_trigger_service().has_order_trigger_action(action_id);
}

kungfu::state<longfist::types::OrderTriggerAction> &Trader::get_order_trigger_action(uint64_t action_id) {
  return get_order_trigger_service().get_order_trigger_action(action_id);
}

bool Trader::has_order_trigger(uint64_t trigger_id) const {
  return get_order_trigger_service().has_order_trigger(trigger_id);
}

state<OrderTrigger> &Trader::get_order_trigger(uint64_t trigger_id) {
  return get_order_trigger_service().get_order_trigger(trigger_id);
}

const OrderTriggerActionMap &Trader::get_order_trigger_actions() const {
  return get_order_trigger_service().get_order_trigger_actions();
}

const AlgoOrderMap &Trader::get_algo_orders() const { return get_algo_order_service().get_algo_orders(); }

bool Trader::has_algo_order(uint64_t order_id) const { return get_algo_order_service().has_algo_order(order_id); }

state<AlgoOrder> &Trader::get_algo_order(uint64_t order_id) {
  return get_algo_order_service().get_algo_order(order_id);
}

const AlgoOrderActionMap &Trader::get_algo_order_actions() const {
  return get_algo_order_service().get_algo_order_actions();
}

void Trader::enable_asset_sync() { sync_asset_ = true; }

void Trader::enable_positions_sync() { sync_position_ = true; }

void Trader::on_asset_sync() {
  if (state_ == BrokerState::Ready) {
    req_account();
  }
}

void Trader::on_position_sync() {
  if (state_ == BrokerState::Ready) {
    req_position();
  }
}

void Trader::recover() {
  deal_write_frame();
  deal_read_frame();
}

void Trader::deal_write_frame() {
  tracer trc(get_home(), false, true, time::today_start(), time::now_in_nano());
  SPDLOG_DEBUG("before tracer read");
  int64_t count = 0;
  while (trc.data_available()) {
    const auto &frame = trc.current_frame();

    switch (frame->msg_type()) {
    case Order::tag: {
      const Order &order = frame->data<Order>();
      get_order_service().on_order(frame->source(), frame->dest(), frame->gen_time(), order);
      break;
    }
    case Trade::tag: {
      const Trade &trade = frame->data<Trade>();
      get_order_service().on_trade(frame->source(), frame->dest(), frame->gen_time(), trade);
      break;
    }
    case OrderTrigger::tag: {
      const OrderTrigger &trigger = frame->data<OrderTrigger>();
      get_order_trigger_service().on_order_trigger(frame->source(), frame->dest(), frame->gen_time(), trigger);
      break;
    }
    case AlgoOrder::tag: {
      const AlgoOrder &algo_order = frame->data<AlgoOrder>();
      get_algo_order_service().on_algo_order(frame->gen_time(), frame->source(), frame->dest(), algo_order);
      break;
    }
    }

    trc.next();
    ++count;
  }
  SPDLOG_DEBUG("after tracer read, count: {}", count);

  get_order_service().clean_orders(disable_recover_);
  get_order_trigger_service().clean_order_triggers(disable_recover_);
  get_algo_order_service().clean_algo_orders(disable_recover_);
}

void Trader::deal_read_frame() {
  // write a Lost Order to journal when read an OrderInput whose order_id not in orders_
  tracer trc(get_home(), true, false, time::today_start(), time::now_in_nano());
  SPDLOG_DEBUG("before tracer read");
  int64_t count = 0;
  while (trc.data_available()) {
    const auto &frame = trc.current_frame();

    switch (frame->msg_type()) {
    case OrderInput::tag: {
      const OrderInput &order_input = frame->data<OrderInput>();
      get_order_service().clean_orders(frame->source(), order_input, disable_recover_);
      break;
    }
    case OrderTriggerInput::tag: {
      const OrderTriggerInput &trigger_input = frame->data<OrderTriggerInput>();
      get_order_trigger_service().clean_order_triggers(frame->source(), trigger_input, disable_recover_);
      break;
    }
    case AlgoOrderInput::tag: {
      const AlgoOrderInput &algo_order_input = frame->data<AlgoOrderInput>();
      get_algo_order_service().clean_algo_orders(frame->source(), algo_order_input, disable_recover_);
      break;
    }
    }

    trc.next();
    ++count;
  }

  SPDLOG_DEBUG("after tracer read, count: {}", count);
}

uint32_t Trader::get_risk_uid() const { return risk_uid_; }

[[maybe_unused]] void Trader::disable_recover() { disable_recover_ = true; }

void Trader::on_risk_setting() {
  const std::string msg = get_risk_setting();
  SPDLOG_DEBUG("RiskSetting: {}", msg);
  auto risk_setting_data = nlohmann::json::parse(msg);
  disable_recover_ = risk_setting_data.value<bool>("disable_recover", false);
  auto risk_check = risk_setting_data.value<bool>("risk_check", false);
  if (risk_check) {
    // let process crash if value is not a json
    auto config = nlohmann::json::parse(risk_setting_data.value<std::string>("value", "{}"));
    const auto risk_name = config.value<std::string>("risk_name", "");
    if (not risk_name.empty()) {
      risk_uid_ = location(get_home()->mode, category::SYSTEM, "service", risk_name, get_home()->locator).location_uid;
    }
  }
}

yijinjing::journal::writer_ptr &Trader::get_thread_writer() {
  return dynamic_cast<TraderVendor &>(get_vendor()).get_thread_writer();
}

void Trader::try_req_account() {
  if (is_sync_account()) {
    req_account();
    disable_sync_account();
  }
}

} // namespace kungfu::wingchun::broker
