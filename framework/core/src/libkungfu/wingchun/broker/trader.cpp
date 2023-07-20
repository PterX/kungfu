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

TraderHook::TraderHook(TraderVendor &vendor) : vendor_(vendor) {}

void TraderHook::pre_write(int64_t trigger_time, const frame_ptr &frame) {}

void TraderHook::post_write(int64_t gen_time, const frame_ptr &frame) {
  switch(frame->msg_type()) {
    case Order::tag: {
      const Order &order = frame->data<Order>();
      get_algo_order_service()->update_algo_order(order);
      break;
    }
  }
}

BrokerService_ptr TraderHook::get_service() {
  return vendor_.get_service();
}

const AlgoOrderService_ptr& TraderHook::get_algo_order_service() {
  return vendor_.get_algo_order_service();
}


TraderVendor::TraderVendor(locator_ptr locator, const std::string &group, const std::string &name, bool low_latency,
                           const std::string &arguments)
    : BrokerVendor(location::make_shared(mode::LIVE, category::TD, group, name, std::move(locator)), low_latency),
      algo_order_service_(std::make_shared<AlgoOrderService>(*this)), hook_(std::make_shared<TraderHook>(*this)) {
  set_arguments(arguments);
}

void TraderVendor::set_service(Trader_ptr service) { service_ = std::move(service); }

void TraderVendor::react() {
  events_ | skip_until(events_ | is(RequestStart::tag)) | is(OrderInput::tag) | $$(service_->handle_order_input(event));
  events_ | skip_until(events_ | is(RequestStart::tag)) | is_custom() | $$(service_->on_custom_event(event));
  apprentice::react();
}

void TraderVendor::on_react() {
  events_ | is(ResetBookRequest::tag) |
      $([&](const event_ptr &event) { get_writer(location::PUBLIC)->mark(now(), ResetBookRequest::tag); });
}

void TraderVendor::on_start() {
  BrokerVendor::on_start();

  events_ | is(BlockMessage::tag) | $$(service_->insert_block_message(event));
  events_ | is(OrderTriggerInput::tag) | $$(service_->insert_order_trigger(event));
  events_ | is(AlgoOrderInput::tag) | $$(algo_order_service_->update_algo_order(event, event->data<AlgoOrderInput>()));
  events_ | is(OrderAction::tag) | $$(service_->cancel_order(event));
  events_ | is(OrderTriggerAction::tag) | $$(service_->cancel_order_trigger(event));
  events_ | is(AlgoOrderAction::tag) | $$(algo_order_service_->cancel_algo_order(event, event->data<AlgoOrderAction>()));
  events_ | is(AssetRequest::tag) | $$(service_->req_account());
  events_ | is(Deregister::tag) | $$(service_->on_strategy_exit(event));
  events_ | is(PositionRequest::tag) | $$(service_->req_position());
  events_ | is(RequestHistoryOrder::tag) | $$(service_->req_history_order(event));
  events_ | is(RequestHistoryTrade::tag) | $$(service_->req_history_trade(event));
  events_ | is(AssetSync::tag) | $$(service_->handle_asset_sync());
  events_ | is(PositionSync::tag) | $$(service_->handle_position_sync());
  events_ | is(Band::tag) | $$(service_->on_band(event));
  events_ | is(TimeKeyValue::tag) | $$(service_->on_time_key_value(event));
  events_ | is(BatchOrderBegin::tag, BatchOrderEnd::tag) | $$(service_->handle_batch_order_tag(event));

  service_->on_risk_setting();
  service_->recover();
  service_->on_recover();
  service_->on_start();
}

void TraderVendor::on_write_to(const event_ptr &event) {
  auto dest_id = event->data<RequestWriteTo>().dest_id;
  if (writers_.find(dest_id) == writers_.end()) {
    writers_.emplace(dest_id, get_io_device()->open_hook_writer(dest_id, hook_));
  }
}

BrokerService_ptr TraderVendor::get_service() { return service_; }

const AlgoOrderService_ptr& TraderVendor::get_algo_order_service() {return algo_order_service_;};

void TraderVendor::clean_orders() {
  std::set<uint32_t> strategy_uids = {};
  auto master_cmd_writer = get_writer(get_master_command_uid());
  for (auto &pair : state_bank_[boost::hana::type_c<Order>]) {
    auto &order_state = pair.second;
    auto &order = const_cast<Order &>(order_state.data);
    auto strategy_uid = order_state.dest;
    if (order.status == OrderStatus::Submitted or order.status == OrderStatus::Pending or
        order.status == OrderStatus::PartialFilledActive) {

      order.status = OrderStatus::Lost;
      order.update_time = time::now_in_nano();

      if (strategy_uid == location::PUBLIC) {
        write_to(now(), order);
        continue;
      }

      strategy_uids.emplace(strategy_uid);

      events_ | is(Channel::tag) | filter([&, strategy_uid](const event_ptr &event) {
        const Channel &channel = event->data<Channel>();
        return channel.source_id == get_home_uid() and channel.dest_id == strategy_uid;
      }) | first() |
          $([this, order, strategy_uid](auto event) { write_to(now(), order, strategy_uid); });
    }
  }
  for (auto uid : strategy_uids) {
    if (not has_writer(uid)) {
      request_write_to(now(), uid);
    }
  }
}

void TraderVendor::on_trading_day(const event_ptr &event, int64_t daytime) { service_->on_trading_day(event, daytime); }

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

void Trader::handle_asset_sync() {
  if (state_ == BrokerState::Ready) {
    req_account();
    write_default_asset_margin();
  }
}

void Trader::handle_position_sync() {
  if (state_ == BrokerState::Ready) {
    req_position();
  }
}

void Trader::handle_order_input(const event_ptr &event) {
  /// try_emplace default insert false to map, means not batch mode
  if (batch_status_.try_emplace(event->source()).first->second) {
    const OrderInput &input = event->data<OrderInput>();
    order_inputs_.try_emplace(event->source()).first->second.push_back(input);
  } else {
    insert_order(event);
  }
}

void Trader::handle_batch_order_tag(const event_ptr &event) {
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
    if (frame->msg_type() == Order::tag) {
      const Order &order = frame->data<Order>();
      orders_.insert_or_assign(order.order_id, state<Order>(frame->source(), frame->dest(), frame->gen_time(), order));
    } else if (frame->msg_type() == Trade::tag) {
      const Trade &trade = frame->data<Trade>();
      trades_.insert_or_assign(trade.trade_id, state<Trade>(frame->source(), frame->dest(), frame->gen_time(), trade));
    }
    trc.next();
    ++count;
  }
  SPDLOG_DEBUG("after tracer read, count: {}", count);

  // set order as Lost which without external_order_id
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
}

void Trader::deal_read_frame() {
  // write a Lost Order to journal when read an OrderInput whose order_id not in orders_
  tracer trc(get_home(), true, false, time::today_start(), time::now_in_nano());
  SPDLOG_DEBUG("before tracer read");
  int64_t count = 0;
  while (trc.data_available()) {
    const auto &frame = trc.current_frame();
    if (frame->msg_type() == OrderInput::tag) {
      const OrderInput &order_input = frame->data<OrderInput>();
      if (orders_.find(order_input.order_id) == orders_.end()) {
        if (has_writer(frame->dest())) {
          Order &order = get_writer(frame->dest())->open_data<Order>();
          order_from_input(order_input, order);
          order.status = OrderStatus::Lost;
          order.update_time = time::now_in_nano();
          get_writer(frame->dest())->close_data();
        }
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
