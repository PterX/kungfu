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
    std::string error_msg = "Not support Algo Order";
    strcpy(algo_order.error_msg, error_msg.c_str());
  }
  writer->close_data();
  return true;
}

[[maybe_unused]] const std::string &Trader::get_account_id() const { return get_home()->name; }

yijinjing::journal::writer_ptr Trader::get_asset_writer() const {
  return get_writer(sync_asset_ ? location::SYNC : location::PUBLIC);
}

yijinjing::journal::writer_ptr Trader::get_asset_margin_writer() const {
  return get_writer(sync_asset_margin_ ? location::SYNC : location::PUBLIC);
}

yijinjing::journal::writer_ptr Trader::get_position_writer() const {
  return get_writer(sync_position_ ? location::SYNC : location::PUBLIC);
}

AlgoOrderService &Trader::get_algo_order_service() {
  return dynamic_cast<TraderVendor &>(get_vendor()).get_algo_order_service();
}

const AlgoOrderService& Trader::get_algo_order_service() const {
  return dynamic_cast<TraderVendor &>(get_vendor()).get_algo_order_service();
}

const AlgoOrderMap &Trader::get_algo_orders() const { return get_algo_order_service().get_algo_orders(); }

void Trader::enable_asset_sync() { sync_asset_ = true; }

void Trader::enable_asset_margin_sync() { sync_asset_margin_ = true; }

void Trader::enable_positions_sync() { sync_position_ = true; }

bool Trader::write_default_asset_margin() {
  SPDLOG_INFO("Write an empty AssetMargin by default");
  sync_asset_margin_ = true;
  auto writer = get_asset_margin_writer();
  AssetMargin &asset_margin = writer->open_data<AssetMargin>();
  asset_margin.holder_uid = get_home_uid();
  asset_margin.update_time = yijinjing::time::now_in_nano();
  writer->close_data();
  return false;
}

void Trader::on_asset_sync() {
  if (state_ == BrokerState::Ready) {
    req_account();
    write_default_asset_margin();
  }
}

void Trader::on_position_sync() {
  if (state_ == BrokerState::Ready) {
    req_position();
  }
}

void Trader::on_order_input(const event_ptr &event) {
  /// try_emplace default insert false to map, means not batch mode
  if (batch_status_.try_emplace(event->source()).first->second) {
    const OrderInput &input = event->data<OrderInput>();
    order_inputs_.try_emplace(event->source()).first->second.push_back(input);
  } else {
    insert_order(event);
  }
}

void Trader::on_batch_order_tag(const event_ptr &event) {
  if (event->msg_type() == BatchOrderBegin::tag) {
    batch_status_.insert_or_assign(event->source(), true);
  } else if (event->msg_type() == BatchOrderEnd::tag) {
    batch_status_.insert_or_assign(event->source(), false);
    insert_batch_orders(event);
    clear_order_inputs(event->source());
  }
}

bool Trader::insert_block_message(const event_ptr &event) {
  const BlockMessage &msg = event->data<BlockMessage>();
  return block_messages_.try_emplace(msg.block_id, msg).second;
}

void Trader::enable_self_detect() { self_deal_detect_ = true; }

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
      orders_.insert_or_assign(order.order_id, state<Order>(frame->source(), frame->dest(), frame->gen_time(), order));
      break;
    }
    case Trade::tag: {
      const Trade &trade = frame->data<Trade>();
      trades_.insert_or_assign(trade.trade_id, state<Trade>(frame->source(), frame->dest(), frame->gen_time(), trade));
      break;
    }
    case OrderTrigger::tag: {
      const OrderTrigger &trigger = frame->data<OrderTrigger>();
      triggers_.insert_or_assign(trigger.trigger_id,
                                 state<OrderTrigger>(frame->source(), frame->dest(), frame->gen_time(), trigger));
      break;
    }
    case AlgoOrder::tag: {
      const AlgoOrder &algo_order = frame->data<AlgoOrder>();
      get_algo_order_service().update_algo_order(frame->gen_time(), frame->source(), frame->dest(), algo_order);
      break;
    }
    }

    trc.next();
    ++count;
  }
  SPDLOG_DEBUG("after tracer read, count: {}", count);

  std::for_each(orders_.begin(), orders_.end(), [&](auto &pair) {
    Order &order = pair.second.data;
    if (not is_final_status(order.status) and (disable_recover_ or order.external_order_id.to_string().empty())) {
      order.status = OrderStatus::Lost;
      order.update_time = time::now_in_nano();
      if (has_writer(pair.second.dest)) {
        write_to(order, pair.second.dest);
      }
    }
  });

  std::for_each(triggers_.begin(), triggers_.end(), [&](auto &pair) {
    OrderTrigger &trigger = pair.second.data;
    if (not is_final_status(trigger.status) and (disable_recover_ or trigger.external_trigger_id.to_string().empty())) {
      trigger.status = OrderStatus::Lost;
      trigger.update_time = time::now_in_nano();
      if (has_writer(pair.second.dest)) {
        write_to(trigger, pair.second.dest);
      }
    }
  });

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
      if (orders_.find(order_input.order_id) == orders_.end()) {
        if (has_writer(frame->source())) {
          Order &order = get_writer(frame->source())->open_data<Order>();
          order_from_input(order_input, order);
          order.status = OrderStatus::Lost;
          order.update_time = time::now_in_nano();
          get_writer(frame->source())->close_data();
        }
      }
      break;
    }
    case OrderTriggerInput::tag: {
      const OrderTriggerInput &trigger_input = frame->data<OrderTriggerInput>();
      if (triggers_.find(trigger_input.trigger_id) == triggers_.end()) {
        if (has_writer(frame->source())) {
          OrderTrigger &trigger = get_writer(frame->source())->open_data<OrderTrigger>();
          order_trigger_from_input(trigger_input, trigger);
          trigger.status = OrderStatus::Lost;
          trigger.update_time = time::now_in_nano();
          get_writer(frame->source())->close_data();
        }
      }
      break;
    }
    case AlgoOrderInput::tag: {
      const auto &algo_order_input = frame->data<AlgoOrderInput>();
      get_algo_order_service().clean_algo_orders(frame->source(), algo_order_input, disable_recover_);
      break;
    }
    }

    trc.next();
    ++count;
  }
  SPDLOG_DEBUG("after tracer read, count: {}", count);
}

void Trader::clear_order_inputs(uint64_t location_uid) { order_inputs_.erase(location_uid); }

[[maybe_unused]] void Trader::disable_recover() { disable_recover_ = true; }

void Trader::on_risk_setting() {
  const std::string msg = get_risk_setting();
  SPDLOG_DEBUG("RiskSetting: {}", msg);
  auto risk_setting_data = nlohmann::json::parse(msg);
  disable_recover_ = risk_setting_data.value<bool>("disable_recover", false);
}

} // namespace kungfu::wingchun::broker
