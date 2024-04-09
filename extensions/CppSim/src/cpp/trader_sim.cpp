#include "trader_sim.h"
#include "common.h"

#include <algorithm>

namespace kungfu::wingchun::sim {
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing;
using namespace longfist::types;
using namespace longfist::enums;

void order_from_order_trigger(const OrderTrigger &trigger, Order &order) {
  order.order_id = trigger.trigger_id;
  order.instrument_id = trigger.instrument_id;     // 合约ID
  order.exchange_id = trigger.exchange_id;         // 交易所ID
  order.instrument_type = trigger.instrument_type; // 合约类型
  order.limit_price = trigger.limit_price;         // 价格
  order.frozen_price = trigger.frozen_price; // 冻结价格, 市价单冻结价格为0
  order.volume = trigger.volume;             // 数量
  order.is_swap = trigger.is_swap;           // 互换单
  order.side = trigger.side;                 // 买卖方向
  order.offset = trigger.offset;             // 开平方向
  order.hedge_flag = trigger.hedge_flag;             // 投机套保标识
  order.price_type = trigger.price_type;             // 价格类型
  order.volume_condition = trigger.volume_condition; // 成交量类型
  order.time_condition = trigger.time_condition;     // 成交时间类型
  order.volume_left = order.volume;
}

TraderSim::TraderSim(broker::BrokerVendor &vendor) : Trader(vendor) {
  KUNGFU_SETUP_LOG();
  SPDLOG_DEBUG("arguments: {}", get_vendor().get_arguments());
}

void TraderSim::pre_start() {
  config_ = nlohmann::json::parse(get_config());
  SPDLOG_INFO("config: {}", get_config());
  if (not config_.recover_order_trade) {
    disable_recover();
  }

  if (mao_order_status.find(config_.match_mode) != mao_order_status.end()) {
    default_status_ = mao_order_status.at(config_.match_mode);
  } else {
    SPDLOG_ERROR("invalid match_mode: {}", int32_t(config_.match_mode));
    default_status_ = OrderStatus::Error;
  }
}

void TraderSim::on_start() { update_broker_state(BrokerState::Ready); }

void TraderSim::on_exit() {}

bool TraderSim::insert_order(const event_ptr &event) {
  const auto &input = event->data<OrderInput>();
  SPDLOG_DEBUG("OrderInput: {}", input.to_string());

  Order order{};
  order_from_input(input, order);
  order.external_order_id = std::to_string(input.order_id).c_str();
  order.insert_time = time::now_in_nano();
  order.update_time = order.insert_time;
  // order.status = OrderStatus::Pending;
  // try_write_to(order, event->source());
  // SPDLOG_DEBUG("Order: {}", order.to_string());

  if (verify_order(order)) {
    // auto submitted_order = order;
    // submitted_order.update_time = time::now_in_nano();
    // submitted_order.status = OrderStatus::Submitted;
    // submitted_order.volume_left = submitted_order.volume;
    // SPDLOG_DEBUG("Submitted Order: {}", submitted_order.to_string());
    // try_write_to(submitted_order, event->source());
    generate_trade(order, event->source());
  }

  order.update_time = time::now_in_nano();
  SPDLOG_DEBUG("Order: {}", order.to_string());
  try_write_to(order, event->source());
  return true;
}

bool TraderSim::cancel_order(const event_ptr &event) {
  const auto &action = event->data<OrderAction>();
  if (not has_order(action.order_id)) {
    OrderActionError error{};
    error.error_id = -1;
    error.error_msg =
        fmt::format("Order of order_id {} not exists", action.order_id).c_str();
    error.order_action_id = action.order_action_id;
    SPDLOG_DEBUG("OrderActionError: {}", error.to_string());
    try_write_to(error, event->source());
    return false;
  }

  if (action.action_flag == OrderActionFlag::Cancel) {
    cancel_order(action.order_id);
  } else {
    OrderTrigger trigger{};
    trigger.trigger_id = action.order_action_id;
    order_trigger_from_order(get_order(action.order_id).data, trigger);
    trigger.insert_time = time::now_in_nano();
    trigger.update_time = trigger.insert_time;
    try_write_to(trigger, event->dest());

    map_trigger_id_to_timer_id_.insert_or_assign(
        trigger.trigger_id,
        add_timer(time::now_in_nano() +
                      int64_t(config_.trigger_delay *
                              time_unit::NANOSECONDS_PER_SECOND),
                  [&, trigger_id = trigger.trigger_id](const auto &) {
                    trigger_start(trigger_id);
                  }));
  }
  return true;
}

bool TraderSim::req_position() {
  static bool skip = false;
  if (skip) {
    return true;
  }
  for (const auto &pos : config_.position_list) {
    auto &po = get_public_writer()->open_data<Position>();
    size_t firstUnderscore = pos.instrument.find('_');
    size_t secondUnderscore = pos.instrument.find('_', firstUnderscore + 1);
    std::string market = pos.instrument.substr(0, firstUnderscore);
    std::string code = pos.instrument.substr(
        firstUnderscore + 1, secondUnderscore - firstUnderscore - 1);
    po.instrument_id = code.c_str();
    po.exchange_id = market.c_str();
    po.yesterday_volume = pos.volume_yesterday;
    po.static_yesterday = pos.volume_yesterday;
    po.open_volume = pos.volume_today;
    po.volume = pos.volume_yesterday + pos.volume_today;
    po.direction = pos.direction;
    po.holder_uid = get_home_uid();
    po.source_id = get_home_uid();
    po.instrument_type = get_instrument_type(po.exchange_id, po.instrument_id);
    po.update_time = time::now_in_nano();
    SPDLOG_DEBUG("Position: {}", po.to_string());
    get_public_writer()->close_data();
  }

  auto &end = get_public_writer()->open_data<PositionEnd>();
  end.holder_uid = get_home_uid();
  SPDLOG_DEBUG("PositionEnd: {}", end.to_string());
  get_public_writer()->close_data();
  skip = true;
  return true;
}

bool TraderSim::req_account() {
  static bool skip = false;
  if (skip) {
    return true;
  }
  auto &asset = get_public_writer()->open_data<Asset>();
  asset.avail = config_.asset;
  asset.avail_margin = config_.asset;
  asset.total_asset = config_.asset;
  asset.holder_uid = get_live_home_uid();
  asset.update_time = time::now_in_nano();
  SPDLOG_DEBUG("Asset: {}", asset.to_string());
  get_public_writer()->close_data();
  skip = true;
  return true;
}

bool TraderSim::req_history_order(const event_ptr &event) { return true; }

bool TraderSim::req_history_trade(const event_ptr &event) { return true; }

void TraderSim::on_recover() {
  //  for (auto &pair : get_orders()) {
  //    SPDLOG_DEBUG("Order: {}", pair.second.data.to_string());
  //    const std::string str_external_order_id =
  //    pair.second.data.external_order_id.to_string(); if (not
  //    str_external_order_id.empty()) {
  //      uint64_t order_id = pair.first;
  //      uint64_t order_xtp_id = std::stoull(str_external_order_id);
  //    }
  //  }
  //  for (auto &pair : get_trades()) {
  //    SPDLOG_DEBUG("Trade: {}", pair.second.data.to_string());
  //    uint64_t order_xtp_id = std::stoull(pair.second.data.external_order_id);
  //  }
}

void TraderSim::generate_trade(const Order &order, uint32_t dest_id) {
  auto fn = [&](double trade_volume) {
    if (has_writer(dest_id)) {
      auto writer = get_writer(dest_id);
      auto &trade = writer->open_data<Trade>();
      trade.trade_id = writer->current_frame_uid();
      trade_from_order(order, trade);
      trade.price = order.limit_price;
      trade.trade_time = time::now_in_nano();
      trade.volume = trade_volume;
      writer->close_data();
    } else {
      Trade trade{};
      trade.trade_id = get_public_writer()->current_frame_uid() xor
                       (time::now_in_nano() & 0xFFFFFFFF);
      trade_from_order(order, trade);
      trade.trade_time = time::now_in_nano();
      trade.volume = trade_volume;
      try_write_to(trade, dest_id);
    }
  };
  switch (config_.match_mode) {
  case MatchMode::Reject:
  case MatchMode::Pending:
  case MatchMode::Cancel:
    return;
  case MatchMode::PartialFillAndCancel:
  case MatchMode::PartialFill: {
    fn(get_min_volume(order));
  } break;
  case MatchMode::Fill:
    fn(order.volume);
    break;
  case MatchMode::MultipleTransactions: {
    double volume_left = order.volume;
    double min_vol = get_min_volume(order);
    while (volume_left != 0) {
      fn(min_vol);
      volume_left -= min_vol;
    }
  } break;
  default:
    SPDLOG_ERROR("invalid match_mode: {}", int32_t(config_.match_mode));
  }
}

bool TraderSim::verify_order(Order &order) {
  double min_vol = get_min_volume(order);
  if (int64_t(order.volume) % int64_t(min_vol) != 0) {
    const std::string msg =
        fmt::format("volume of InstrumentType {} must be multiple of {}",
                    nlohmann::json(get_instrument_type(order.exchange_id,
                                                       order.instrument_id)),
                    min_vol);
    order.status = OrderStatus::Error;
    order.error_id = -1;
    order.error_msg = msg.c_str();
    return false;
  }
  order.status = default_status_;

  switch (config_.match_mode) {
  case MatchMode::Reject:
  case MatchMode::Pending:
  case MatchMode::Cancel:
    return false;
  case MatchMode::PartialFillAndCancel:
  case MatchMode::PartialFill:
    order.volume_left -= min_vol;
    if (order.volume_left == 0) {
      order.status = OrderStatus::Filled;
    }
    break;
  case MatchMode::Fill:
  case MatchMode::MultipleTransactions: {
    order.volume_left = 0;
  } break;
  }
  return true;
}

void TraderSim::trigger_start(uint64_t trigger_id) {
  if (not has_order_trigger(trigger_id)) {
    SPDLOG_ERROR("no OrderTrigger of trigger_id: {}", trigger_id);
    return;
  }

  auto &trigger_state = get_order_trigger(trigger_id);
  trigger_state.data.status = OrderStatus::Filled;
  trigger_state.update_time = time::now_in_nano();
  try_write_to(trigger_state.data, trigger_state.dest);

  if (trigger_state.data.action_flag == OrderTriggerFlag::TriggerCancel) {
    cancel_order(trigger_state.data.order_id);
  } else if (trigger_state.data.action_flag ==
             OrderTriggerFlag::TriggerInsert) {
    Order order{};
    order_from_order_trigger(trigger_state.data, order);
    order.external_order_id = std::to_string(order.order_id).c_str();
    order.insert_time = now();
    order.update_time = now();
    order.status = OrderStatus::Pending;
    try_write_to(order, trigger_state.dest);
    SPDLOG_DEBUG("Order: {}", order.to_string());

    if (verify_order(order)) {
      auto submitted_order = order;
      submitted_order.update_time = time::now_in_nano();
      submitted_order.status = OrderStatus::Submitted;
      submitted_order.volume_left = submitted_order.volume;
      SPDLOG_DEBUG("Submitted Order: {}", submitted_order.to_string());
      try_write_to(submitted_order, trigger_state.dest);
      generate_trade(order, trigger_state.dest);
    }
    order.update_time = time::now_in_nano();
    SPDLOG_DEBUG("Order: {}", order.to_string());
    try_write_to(order, trigger_state.dest);
  }
}

void TraderSim::cancel_order(uint64_t order_id) {
  if (not has_order(order_id)) {
    return;
  }
  auto &order_state = get_order(order_id);
  order_state.data.status =
      order_state.data.volume_left == order_state.data.volume
          ? OrderStatus::Cancelled
          : OrderStatus::PartialFilledNotActive;
  order_state.data.update_time = time::now_in_nano();
  try_write_to(order_state.data, order_state.dest);
  // order_state.data.status = OrderStatus::Cancelling;
  // order_state.data.update_time = time::now_in_nano();
  // SPDLOG_DEBUG("Order: {}", order_state.data.to_string());
  // try_write_to(order_state.data, order_state.dest);
  // add_timer(time::now_in_nano() + int64_t(config_.cancel_delay *
  //                                         time_unit::NANOSECONDS_PER_SECOND),
  //           [&](const auto &) {
  //             order_state.data.status =
  //                 order_state.data.volume_left == order_state.data.volume
  //                     ? OrderStatus::Cancelled
  //                     : OrderStatus::PartialFilledNotActive;
  //             order_state.data.update_time = time::now_in_nano();
  //             try_write_to(order_state.data, order_state.dest);
  //           });
}

bool TraderSim::insert_order_trigger(const event_ptr &event) {
  const OrderTriggerInput &trigger_input = event->data<OrderTriggerInput>();
  SPDLOG_DEBUG("OrderTriggerInput: {}", trigger_input.to_string());
  OrderTrigger trigger{};
  order_trigger_from_input(trigger_input, trigger);
  trigger.insert_time = time::now_in_nano();
  trigger.update_time = trigger.insert_time;
  SPDLOG_DEBUG("OrderTrigger: {}", trigger.to_string());
  try_write_to(trigger, event->source());

  map_trigger_id_to_timer_id_.insert_or_assign(
      trigger.trigger_id,
      add_timer(time::now_in_nano() +
                    int64_t(config_.trigger_delay *
                            time_unit::NANOSECONDS_PER_SECOND),
                [&, trigger_id = trigger.trigger_id](const auto &) {
                  trigger_start(trigger_id);
                }));
  return true;
}

bool TraderSim::cancel_order_trigger(const event_ptr &event) {
  auto &action = event->data<OrderTriggerAction>();
  if (map_trigger_id_to_timer_id_.find(action.trigger_id) ==
          map_trigger_id_to_timer_id_.end() or
      not has_order_trigger(action.trigger_id)) {
    OrderTriggerActionError error{};
    error.trigger_id = action.trigger_id;
    error.error_id = -1;
    error.error_msg = fmt::format("OrderTrigger of trigger_id {} not exists",
                                  action.trigger_id)
                          .c_str();
    error.order_trigger_action_id = action.order_trigger_action_id;
    SPDLOG_DEBUG("OrderTriggerActionError: {}", error.to_string());
    try_write_to(error, event->source());
    return false;
  }

  clear_timer(map_trigger_id_to_timer_id_.at(action.trigger_id));
  auto &trigger_state = get_order_trigger(action.trigger_id);
  trigger_state.data.status = OrderStatus::Cancelled;
  SPDLOG_DEBUG("OrderTrigger: {}", trigger_state.data.to_string());
  try_write_to(trigger_state.data, trigger_state.dest);
  return true;
}

} // namespace kungfu::wingchun::sim
