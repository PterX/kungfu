// SPDX-License-Identifier: Apache-2.0

//
// Created by marsjliu on 2023/4/11.
//

#ifndef WINGCHUN_ACCOUNTING_STOCK_LONG_SHORT_H
#define WINGCHUN_ACCOUNTING_STOCK_LONG_SHORT_H

#include <exception>
#include <kungfu/wingchun/book/accounting.h>
#include <kungfu/wingchun/book/bookkeeper.h>
#include <math.h>
#include <mutex>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;

namespace kungfu::wingchun::book {

struct otc_contract_discount_and_margin_ratio {
  int32_t contract_multiplier;
  double long_margin_ratio;
  double short_margin_ratio;
  double margin_ratio;
  double conversion_rate;
  double exchange_rate;
};
class OtcStockAccountingMethod : public AccountingMethod {
public:
  static constexpr int DEFAULT_OTC_STOCK_CONTRACT_MULTIPLIER = 1;
  static constexpr float DEFAULT_OTC_STOCK_LONG_MARGIN_RATIO = 1.0;
  static constexpr float DEFAULT_OTC_STOCK_SHORT_MARGIN_RATIO = 0.6;
  static constexpr float DEFAULT_OTC_STOCK_CONVERSION_RATE = 0.7;
  static constexpr int OTC_AMOUT_PRECISION = 3;
  static constexpr double OTC_MAX_COLLATERAL_RATIO = 1000.0;
  static constexpr double OTC_DEFAULT_STOCK_EXCHANGE_RATE = 1.0;

  OtcStockAccountingMethod() = default;

  virtual void apply_trading_day(Book_ptr &book, int64_t trading_day) override {
    auto apply = [&](PositionMap &positions) {
      for (auto &pair : positions) {
        auto &position = pair.second;
        //[[maybe_unused]] auto margin_pre = position.margin;
        if (is_valid_price(position.close_price)) {
          position.pre_close_price = position.close_price;
        } else if (is_valid_price(position.last_price)) {
          position.pre_close_price = position.last_price;
        }
        // collateral; security
        auto cd_mr = get_instr_conversion_margin_rate(book, position);
        auto margin_ratio =
            (position.direction == Direction::Long ? cd_mr.long_margin_ratio : cd_mr.short_margin_ratio);

        if (position.direction == Direction::Short) {
          position.margin = position.pre_close_price * cd_mr.exchange_rate * position.volume * margin_ratio;
        }

        position.yesterday_volume = position.volume;
        position.close_price = 0;
        position.update_time = trading_day;
        position.frozen_total = 0;
        position.frozen_yesterday = 0;
        position.trading_day = time::strftime(trading_day, KUNGFU_TRADING_DAY_FORMAT).c_str();

        update_position(book, position);
      }
    };

    apply(book->long_positions);
    apply(book->short_positions);
  }

  virtual void apply_quote(Book_ptr &book, const Quote &quote) override {
    static int counter = 0;
    auto apply = [&](Position &position) {
      if (not is_valid_price(quote.last_price) or not position.volume) {
        return;
      }

      if (not position.last_price) {
        position.last_price = quote.last_price;
      }
      double price_change = quote.last_price - position.last_price;
      position.last_price = quote.last_price;

      auto cd_mr = get_instr_conversion_margin_rate(book, position);
      double market_value_change = price_change * cd_mr.exchange_rate * position.volume;

      auto &asset = book->asset;

      if (position.direction == Direction::Long) {
        asset.market_value += market_value_change;
        asset.unrealized_pnl += market_value_change;
      } else {
        SPDLOG_DEBUG("OtcStockAccountingMethod: apply_quote  Direction::Short instrument_id= {}",
                     position.instrument_id);
      }

      update_position(book, position);
      if (counter > 20) {
        counter = 0;
        calculate_marketvalue(book);
      }
    };
    apply(book->get_position_for(Direction::Long, quote));
    apply(book->get_position_for(Direction::Short, quote));
    ++counter;
  }

  virtual void apply_order_input(Book_ptr &book, const OrderInput &input) override {
    auto &position = book->get_position_for(input);
    auto cd_mr = get_instr_conversion_margin_rate(book, position);
    // Offset: Close
    if (input.side == Side::Sell) {
      position.frozen_total += input.volume;
      if (position.yesterday_volume - position.frozen_yesterday >= input.volume) {
        position.frozen_yesterday += input.volume;
      } else {
        position.frozen_yesterday = position.yesterday_volume;
      }
    } else if (input.side == Side::Buy) { // Offset: Open
      double frozen_cash = input.volume * input.frozen_price * cd_mr.exchange_rate * cd_mr.margin_ratio;
      book->asset.frozen_cash += frozen_cash;
      book->asset.avail -= frozen_cash;
    }
  }

  virtual void apply_order(Book_ptr &book, const Order &order) override {
    if (is_final_status(order.status)) {
      auto &position = book->get_position_for(order);
      auto cd_mr = get_instr_conversion_margin_rate(book, position);
      auto &asset = book->asset;
      if (order.side == Side::Buy) {
        auto frozen =
            book->get_frozen_price(order.order_id) * order.volume_left * cd_mr.exchange_rate * cd_mr.margin_ratio;
        asset.frozen_cash -= frozen;
        asset.avail += frozen;
      } else if (order.side == Side::Sell) {
        position.frozen_total = std::max(position.frozen_total - order.volume_left, VOLUME_ZERO);
        position.frozen_yesterday = std::max(position.frozen_yesterday - order.volume_left, VOLUME_ZERO);
      }

      update_position(book, position);
    }
  }

  virtual void apply_trade(Book_ptr &book, const Trade &trade) override {
    if (trade.side == Side::Sell) {
      apply_sell(book, trade);
    } else if (trade.side == Side::Buy) {
      apply_buy(book, trade);
    }
  }

  virtual void update_position(Book_ptr &book, Position &position) override {
    if (position.last_price > 0) {
      double price_change = position.last_price - position.avg_open_price;
      position.unrealized_pnl =
          (position.direction == Direction::Long ? price_change : -price_change) * position.volume;
    }
  }

protected:
  std::unordered_map<uint64_t, double> commission_map_ = {};
  std::mutex accounting_mutex_;
  [[maybe_unused]] double short_market_value_ = 0;
  [[maybe_unused]] double long_market_value_ = 0;

  virtual void calculate_marketvalue(Book_ptr &book) {
    double short_market_value = 0;
    double long_market_value = 0;

    auto apply = [&](PositionMap &positions, double &market_value) {
      for (auto &pair : positions) {
        auto &position = pair.second;
        //        auto margin_pre = position.margin;
        auto cd_mr = get_instr_conversion_margin_rate(book, position);
        if (is_valid_price(position.last_price)) {
          market_value += position.volume * position.last_price * cd_mr.exchange_rate;
        } else {
          if (is_valid_price(position.pre_close_price)) {
            market_value += position.volume * position.pre_close_price * cd_mr.exchange_rate;
          } else if (is_valid_price(position.avg_open_price)) {
            market_value += position.volume * position.avg_open_price * cd_mr.exchange_rate;
          }
        }
        position.update_time = yijinjing::time::now_in_nano();
        update_position(book, position);
      }
    };

    apply(book->long_positions, long_market_value);
    long_market_value_ = long_market_value;
    apply(book->short_positions, short_market_value);
    short_market_value_ = short_market_value;
  }

  virtual void apply_buy(Book_ptr &book, const Trade &trade) {
    auto &position = book->get_position_for(trade);
    auto cd_mr = get_instr_conversion_margin_rate(book, position);
    double trade_amt = trade.price * trade.volume * cd_mr.exchange_rate;
    double commission = calculate_commission(trade);
    double tax = calculate_tax(trade);
    position.last_price = position.last_price > 0 ? position.last_price : trade.price;
    if (position.volume + trade.volume > 0 && trade.price > 0) {
      position.avg_open_price = (position.avg_open_price * position.volume + trade_amt / cd_mr.exchange_rate) /
                                (double)(position.volume + trade.volume);
      position.position_cost_price =
          (position.position_cost_price * position.volume + trade_amt / cd_mr.exchange_rate + commission + tax) /
          (double)(position.volume + trade.volume);
    }
    double unrealized_pnl_change = (position.last_price - trade.price) * trade.volume;
    position.volume += trade.volume;
    position.unrealized_pnl += unrealized_pnl_change;

    auto &asset = book->asset;

    double frozen_cash_to_release =
        book->get_frozen_price(trade.order_id) * cd_mr.exchange_rate * trade.volume * cd_mr.margin_ratio;
    asset.frozen_cash -= frozen_cash_to_release;
    double avail_cash_change = frozen_cash_to_release - trade_amt * cd_mr.margin_ratio - (commission + tax);
    asset.avail += avail_cash_change;
    asset.intraday_fee += commission + tax;
    asset.accumulated_fee += commission + tax;
    asset.unrealized_pnl += unrealized_pnl_change * cd_mr.exchange_rate;
    double position_market_value_change = trade.volume * position.last_price * cd_mr.exchange_rate;
    asset.market_value += position_market_value_change;
  }

  virtual void apply_sell(Book_ptr &book, const Trade &trade) {
    auto &position = book->get_position_for(trade);
    auto cd_mr = get_instr_conversion_margin_rate(book, position);
    double commission = calculate_commission(trade);
    double tax = calculate_tax(trade);
    position.frozen_total = std::max(position.frozen_total - trade.volume, VOLUME_ZERO);
    position.frozen_yesterday = std::max(position.frozen_yesterday - trade.volume, VOLUME_ZERO);
    position.yesterday_volume = std::max(position.yesterday_volume - trade.volume, VOLUME_ZERO);
    position.volume = std::max(position.volume - trade.volume, VOLUME_ZERO);
    double realized_pnl = (trade.price - position.avg_open_price) * trade.volume;
    position.realized_pnl += realized_pnl;

    double unrealized_pnl_change = (position.last_price - position.avg_open_price) * trade.volume;
    position.unrealized_pnl -= unrealized_pnl_change;
    // position.unrealized_pnl -= realized_pnl;

    update_position(book, position);
    auto &asset = book->asset;
    double trade_amt = trade.price * cd_mr.exchange_rate * trade.volume;
    double repay_cash_debt = std::min(position.margin, (trade_amt * cd_mr.margin_ratio - (commission + tax)));
    double cash_delivery = trade_amt * cd_mr.margin_ratio - repay_cash_debt - (commission + tax);

    asset.realized_pnl += realized_pnl * cd_mr.exchange_rate;
    asset.unrealized_pnl -= realized_pnl * cd_mr.exchange_rate; // unrealized_pnl_change
    asset.avail += cash_delivery;
    asset.market_value -= trade_amt;
    asset.intraday_fee += commission + tax;
    asset.accumulated_fee += commission + tax;
  }

  virtual double calculate_commission(const Trade &trade) { return trade.commission; }

  virtual double calculate_tax(const Trade &trade) { return trade.tax; }

  static otc_contract_discount_and_margin_ratio get_instr_conversion_margin_rate(const Book_ptr &book,
                                                                                 const Position &position) {
    const char *exchange_id = position.exchange_id;
    const char *instrument_id = position.instrument_id;
    uint32_t hashed_instrument_key = hash_instrument(exchange_id, instrument_id);
    otc_contract_discount_and_margin_ratio cd_mr = {};

    if (book->instruments.find(hashed_instrument_key) == book->instruments.end()) {
      cd_mr.contract_multiplier = DEFAULT_OTC_STOCK_CONTRACT_MULTIPLIER;
      cd_mr.margin_ratio = position.direction == Direction::Long ? DEFAULT_OTC_STOCK_LONG_MARGIN_RATIO
                                                                 : DEFAULT_OTC_STOCK_SHORT_MARGIN_RATIO;
      cd_mr.long_margin_ratio = DEFAULT_OTC_STOCK_LONG_MARGIN_RATIO;
      cd_mr.short_margin_ratio = DEFAULT_OTC_STOCK_SHORT_MARGIN_RATIO;
      cd_mr.conversion_rate = DEFAULT_OTC_STOCK_CONVERSION_RATE;
      cd_mr.exchange_rate = OTC_DEFAULT_STOCK_EXCHANGE_RATE;
      return cd_mr;
    }
    try {
      auto &instrument = book->instruments.at(hashed_instrument_key);
      cd_mr.contract_multiplier = instrument.contract_multiplier;
      cd_mr.margin_ratio = margin_ratio(instrument, position);
      cd_mr.long_margin_ratio = instrument.long_margin_ratio;
      cd_mr.short_margin_ratio = instrument.short_margin_ratio;
      cd_mr.conversion_rate = instrument.conversion_rate;
      cd_mr.exchange_rate = is_equal(instrument.exchange_rate, 0.0) ? 1.0 : instrument.exchange_rate;
    } catch (std::exception &ex) {
      SPDLOG_ERROR("Exception for instrument_id {}: {}", instrument_id, ex.what());
      cd_mr.margin_ratio = position.direction == Direction::Long ? DEFAULT_OTC_STOCK_LONG_MARGIN_RATIO
                                                                 : DEFAULT_OTC_STOCK_SHORT_MARGIN_RATIO;
      cd_mr.long_margin_ratio = DEFAULT_OTC_STOCK_LONG_MARGIN_RATIO;
      cd_mr.short_margin_ratio = DEFAULT_OTC_STOCK_SHORT_MARGIN_RATIO;
      cd_mr.conversion_rate = DEFAULT_OTC_STOCK_CONVERSION_RATE;
      cd_mr.exchange_rate = OTC_DEFAULT_STOCK_EXCHANGE_RATE;
    }
    return cd_mr;
  }

  static double margin_ratio(const Instrument &instrument, const Position &position) {
    return position.direction == Direction::Long ? instrument.long_margin_ratio : instrument.short_margin_ratio;
  }
  [[maybe_unused]] static double roundn(double value, int n = OTC_AMOUT_PRECISION) {
    double x = pow(10.0, (double)n);
    double round_val = round(value * x) / x;
    return round_val;
  }
};
} // namespace kungfu::wingchun::book
#endif // WINGCHUN_ACCOUNTING_STOCK_LONG_SHORT_H
