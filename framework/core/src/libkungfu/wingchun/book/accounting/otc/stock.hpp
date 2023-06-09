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

struct otc_stock_contract_multiplier_and_margin_ratio {
  int32_t contract_multiplier;
  double long_margin_ratio;
  double short_margin_ratio;
  double margin_ratio;
  double exchange_rate;
};
class OtcStockAccountingMethod : public AccountingMethod {
public:
  static constexpr int DEFAULT_OTC_STOCK_CONTRACT_MULTIPLIER = 1;
  static constexpr float DEFAULT_OTC_STOCK_LONG_MARGIN_RATIO = 1.0;
  static constexpr float DEFAULT_OTC_STOCK_SHORT_MARGIN_RATIO = 0.6;
  static constexpr int OTC_AMOUT_PRECISION = 3;
  static constexpr double OTC_MAX_COLLATERAL_RATIO = 1000.0;
  static constexpr double OTC_DEFAULT_STOCK_EXCHANGE_RATE = 1.0;

  OtcStockAccountingMethod() = default;

  virtual void apply_trading_day(Book_ptr &book, int64_t trading_day) override {
    auto apply = [&](auto &position) {
      if (is_valid_price(position.close_price)) {
        position.pre_close_price = position.close_price;
      } else if (is_valid_price(position.last_price)) {
        position.pre_close_price = position.last_price;
      }
      auto cd_mr = get_instrument_conversion_margin_rate(book, position.source_id, position.direction,
                                                         position.exchange_id, position.instrument_id);
      auto margin_ratio = (position.direction == Direction::Long ? cd_mr.long_margin_ratio : cd_mr.short_margin_ratio);

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
    };

    book->apply_long_positions(apply);
    book->apply_short_positions(apply);
  }

  virtual void apply_quote(Book_ptr &book, const Quote &quote) override {
    auto apply = [&](auto &position) {
      if (not is_valid_price(quote.last_price) or not position.volume) {
        return;
      }

      if (not position.last_price) {
        position.last_price = quote.last_price;
      }
      double price_change = quote.last_price - position.last_price;
      position.last_price = quote.last_price;

      auto cd_mr = get_instrument_conversion_margin_rate(book, position.source_id, position.direction,
                                                         position.exchange_id, position.instrument_id);
      double market_value_change = price_change * cd_mr.exchange_rate * position.volume;

      if (position.direction == Direction::Long) {
        book->asset.market_value += market_value_change;
        book->asset.unrealized_pnl += market_value_change;
      } else {
        SPDLOG_DEBUG("OtcStockAccountingMethod: apply_quote  Direction::Short instrument_id= {}",
                     position.instrument_id);
      }

      update_position(book, position);
    };

    book->apply_long_position_for(quote, apply);
    book->apply_short_position_for(quote, apply);
  }

  virtual void apply_order_input(Book_ptr &book, uint32_t account_id, const OrderInput &input) override {
    auto apply = [&](auto &position) {
      if (input.side == Side::Buy) { // Offset: Open
        auto cd_mr = get_instrument_conversion_margin_rate(book, account_id, position.direction, input.exchange_id,
                                                           input.instrument_id);
        double frozen_cash = input.volume * input.frozen_price * cd_mr.exchange_rate * cd_mr.margin_ratio;
        book->asset.frozen_cash += frozen_cash;
        book->asset.avail -= frozen_cash;
        return;
      } else if (input.side == Side::Sell) {
        position.frozen_total += input.volume;
        if (position.yesterday_volume - position.frozen_yesterday >= input.volume) {
          position.frozen_yesterday += input.volume;
        } else {
          position.frozen_yesterday = position.yesterday_volume;
        }
      };
    };

    book->apply_position_for(account_id, input, apply);
  }

  virtual void apply_order(Book_ptr &book, uint32_t account_id, const Order &order) override {
    if (not is_final_status(order.status))
      return;

    auto apply = [&](auto &position) {
      if (order.side == Side::Buy) {
        auto cd_mr = get_instrument_conversion_margin_rate(book, account_id, position.direction, order.exchange_id,
                                                           order.instrument_id);
        auto frozen =
            book->get_frozen_price(order.order_id) * order.volume_left * cd_mr.exchange_rate * cd_mr.margin_ratio;
        book->asset.frozen_cash -= frozen;
        book->asset.avail += frozen;
      }

      if (order.side == Side::Sell) {
        auto apply = [&](auto &position) {
          position.frozen_total = std::max(position.frozen_total - order.volume_left, VOLUME_ZERO);
          position.frozen_yesterday = std::max(position.frozen_yesterday - order.volume_left, VOLUME_ZERO);
        };
        book->apply_position_for(account_id, order, apply);
      }

      update_position(book, position);
    };

    book->apply_position_for(account_id, order, apply);
  }

  virtual void apply_trade(Book_ptr &book, uint32_t account_id, const Trade &trade) override {
    auto apply = [&](auto &position) {
      if (trade.side == Side::Sell) {
        apply_sell(book, position, trade);
      } else if (trade.side == Side::Buy) {
        apply_buy(book, position, trade);
      }
    };

    book->apply_position_for(account_id, trade, apply);
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

  virtual void apply_buy(Book_ptr &book, longfist::types::Position &position, const Trade &trade) {
    auto cd_mr = get_instrument_conversion_margin_rate(book, position.source_id, position.direction,
                                                       position.exchange_id, position.instrument_id);
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

  virtual void apply_sell(Book_ptr &book, longfist::types::Position &position, const Trade &trade) {
    auto cd_mr = get_instrument_conversion_margin_rate(book, position.source_id, position.direction,
                                                       position.exchange_id, position.instrument_id);
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

  static otc_stock_contract_multiplier_and_margin_ratio
  get_instrument_conversion_margin_rate(const Book_ptr &book, uint32_t account_id, longfist::enums::Direction direction,
                                        const char *exchange_id, const char *instrument_id) {
    uint32_t hashed_instrument_key = hash_instrument(exchange_id, instrument_id);
    otc_stock_contract_multiplier_and_margin_ratio cd_mr = {};

    if (book->instruments.find(hashed_instrument_key) == book->instruments.end()) {
      cd_mr.contract_multiplier = DEFAULT_OTC_STOCK_CONTRACT_MULTIPLIER;
    } else {
      auto &instrument = book->instruments.at(hashed_instrument_key);
      cd_mr.contract_multiplier = instrument.contract_multiplier;
    }

    auto hashed_instrument_factor_key = hash_instrument(account_id, exchange_id, instrument_id);
    if (book->instrument_factors.find(hashed_instrument_factor_key) == book->instrument_factors.end()) {
      cd_mr.margin_ratio =
          direction == Direction::Long ? DEFAULT_OTC_STOCK_LONG_MARGIN_RATIO : DEFAULT_OTC_STOCK_SHORT_MARGIN_RATIO;
      cd_mr.long_margin_ratio = DEFAULT_OTC_STOCK_LONG_MARGIN_RATIO;
      cd_mr.short_margin_ratio = DEFAULT_OTC_STOCK_SHORT_MARGIN_RATIO;
      cd_mr.exchange_rate = OTC_DEFAULT_STOCK_EXCHANGE_RATE;
    } else {
      auto &factor = book->instrument_factors.at(hashed_instrument_factor_key);
      cd_mr.margin_ratio = direction == Direction::Long ? factor.long_margin_ratio : factor.short_margin_ratio;
      cd_mr.long_margin_ratio = factor.long_margin_ratio;
      cd_mr.short_margin_ratio = factor.short_margin_ratio;
      cd_mr.exchange_rate = is_equal(factor.exchange_rate, 0.0) ? 1.0 : factor.exchange_rate;
    }

    return cd_mr;
  }

  [[maybe_unused]] static double roundn(double value, int n = OTC_AMOUT_PRECISION) {
    double x = pow(10.0, (double)n);
    double round_val = round(value * x) / x;
    return round_val;
  }
};
} // namespace kungfu::wingchun::book
#endif // WINGCHUN_ACCOUNTING_STOCK_LONG_SHORT_H
