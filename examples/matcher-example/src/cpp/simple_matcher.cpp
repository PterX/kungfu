#include <kungfu/wingchun/common.h>
#include <kungfu/wingchun/extension.h>
#include <kungfu/wingchun/strategy/matcher.h>
#include <spdlog/spdlog.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;
using kungfu::wingchun::get_direction;
using kungfu::wingchun::hash_instrument;
using kungfu::wingchun::order_from_input;
using kungfu::yijinjing::time;

KUNGFU_MAIN_MATCHER(SimpleMatcher) {
public:
  SimpleMatcher() = default;
  ~SimpleMatcher() = default;

  typedef std::unordered_map<uint64_t, Order> OrderMap;
  // typedef std::unordered_map<longfist::types::InstrumentKey, longfist::types::Quote> QuoteMap;
  typedef std::unordered_map<uint32_t, Quote> QuoteMap;

  virtual void on_quote(const Quote &quote) override {
    // InstrumentKey instrument_key{};
    // instrument_key.instrument_id = quote.instrument_id;
    // instrument_key.exchange_id = quote.exchange_id;
    // instrument_key.instrument_type = quote.instrument_type;
    quotes_[hash_instrument(quote.exchange_id, quote.instrument_id)] = quote;
    match();
  }

  virtual void on_order_input(const OrderInput &order_input) override {
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

  virtual void on_order_action(const OrderAction &order_action) override {
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

  void match() {
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

  void filled_order_trade(Order & order, Trade & trade) {
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

private:
  QuoteMap quotes_;
  OrderMap orders_;
};
