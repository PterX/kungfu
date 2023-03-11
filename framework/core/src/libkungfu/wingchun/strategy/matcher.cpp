// SPDX-License-Identifier: Apache-2.0

#include <fmt/format.h>

#include <kungfu/wingchun/strategy/matcher.h>
#include <kungfu/wingchun/strategy/runner.h>
#include <kungfu/yijinjing/log.h>
#include <kungfu/yijinjing/time.h>

// using namespace kungfu::yijinjing::practice;
using namespace kungfu::rx;
using namespace kungfu::longfist;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::util;

namespace kungfu::wingchun::strategy {
void Matcher::update_order(const Order &order) {
  auto writer = app_->get_writer(location::PUBLIC);
  writer->write_at(app_->now(), app_->now(), order);
}

void Matcher::update_trade(const Trade &trade) {
  auto writer = app_->get_writer(location::PUBLIC);
  writer->write_at(app_->now(), app_->now(), trade);
}

void Matcher::update_order_action_error(const OrderActionError &error) {
  auto writer = app_->get_writer(location::PUBLIC);
  writer->write_at(app_->now(), app_->now(), error);
}

void set_runner(Matcher &matcher, Runner *runner) { matcher.app_ = runner; }

void BasicMatcher::on_quote(const Quote &quote) {
  // InstrumentKey instrument_key{};
  // instrument_key.instrument_id = quote.instrument_id;
  // instrument_key.exchange_id = quote.exchange_id;
  // instrument_key.instrument_type = quote.instrument_type;
  quotes_[hash_instrument(quote.exchange_id, quote.instrument_id)] = quote;
  match();
};

void BasicMatcher::on_order_input(const OrderInput &order_input) {
  Order order{};
  order_from_input(order_input, order);
  if (order.price_type != PriceType::Limit) {
    order.status = OrderStatus::Error;
    order.error_id = 1;
    order.error_msg = "only limit order supported";
    update_order(order);
    return;
  } else {
    order.status = OrderStatus::Submitted;
    update_order(order);
  }
  auto direction = get_direction(order.instrument_type, order.side, order.offset);
  if (quotes_.find(hash_instrument(order.exchange_id, order.instrument_id)) == quotes_.end()) {
    return;
  }
  const auto &quote = quotes_[hash_instrument(order.exchange_id, order.instrument_id)];
  if (direction == Direction::Long and order.limit_price > quote.ask_price[0]) {
    Trade trade{};
    filled_order_trade(order, trade);
    trade.price = quote.ask_price[0];
    update_order(order);
    update_trade(trade);
  } else if (direction == Direction::Short and order.limit_price < quote.bid_price[0]) {
    Trade trade{};
    filled_order_trade(order, trade);
    trade.price = quote.bid_price[0];
    update_order(order);
    update_trade(trade);
  } else {
    orders_[order.order_id] = order;
  }
}

void BasicMatcher::on_order_action(const OrderAction &order_action) {
  if (orders_.find(order_action.order_id) != orders_.end()) {
    Order order = orders_[order_action.order_id];
    order.status = OrderStatus::Cancelled;
    update_order(order);
    orders_.erase(order_action.order_id);
  } else {
    OrderActionError error{};
    error.order_id = order_action.order_id;
    error.error_id = 1;
    error.error_msg = fmt::format("order {} not found", order_action.order_id).c_str();
    // error.insert_time = order_action.insert_time;
    update_order_action_error(error);
  }
}

void BasicMatcher::filled_order_trade(Order &order, Trade &trade) {
  order.status = OrderStatus::Filled;
  order.update_time = now();
  order.volume_left = 0;
  // order.commision = 0;
  // order.tax = 0;
  trade.trade_id = order.order_id;
  trade.order_id = order.order_id;
  trade.trade_time = order.update_time;
  trade.instrument_id = order.instrument_id;
  trade.exchange_id = order.exchange_id;
  trade.instrument_type = order.instrument_type;
  trade.side = order.side;
  trade.offset = order.offset;
  trade.hedge_flag = order.hedge_flag;
  trade.price = order.limit_price;
  trade.volume = order.volume;
  // trade.tax = 0;
  // trade.commision = 0;
}

void BasicMatcher::match() {
  auto order_it = orders_.begin();
  while (order_it != orders_.end()) {
    auto &order = order_it->second;
    auto direction = get_direction(order.instrument_type, order.side, order.offset);
    if (quotes_.find(hash_instrument(order.exchange_id, order.instrument_id)) == quotes_.end()) {
      order_it++;
      continue;
    }
    const auto &quote = quotes_[hash_instrument(order.exchange_id, order.instrument_id)];

    if ((direction == Direction::Long and order.limit_price > quote.ask_price[0]) or
        (direction == Direction::Short and order.limit_price < quote.bid_price[0])) {
      Trade trade{};
      filled_order_trade(order, trade);
      update_order(order);
      update_trade(trade);
      order_it = orders_.erase(order_it);
      continue;
    }
    order_it++;
  }
}
} // namespace kungfu::wingchun::strategy
