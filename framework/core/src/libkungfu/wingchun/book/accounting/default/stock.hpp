// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/4/6.
// Updated for Margin Account on 2022/6/6
//

#ifndef WINGCHUN_ACCOUNTING_STOCK_H
#define WINGCHUN_ACCOUNTING_STOCK_H

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

struct contract_discount_and_margin_ratio {
  int32_t contract_multiplier;
  double long_margin_ratio;
  double short_margin_ratio;
  double margin_ratio;
  double conversion_rate; // For collateral/avail_margin calculation
  double exchange_rate;   // 汇率
};
class StockAccountingMethod : public AccountingMethod {
public:
  static constexpr int DEFAULT_STOCK_CONTRACT_MULTIPLIER = 1;
  static constexpr float DEFAULT_STOCK_LONG_MARGIN_RATIO = 1.0;
  static constexpr float DEFAULT_STOCK_SHORT_MARGIN_RATIO = 0.6;
  static constexpr float DEFAULT_STOCK_CONVERSION_RATE = 0.7;
  static constexpr int AMOUT_PRECISION = 3;
  static constexpr double MAX_COLLATERAL_RATIO = 1000.0;
  static constexpr double DEFAULT_STOCK_EXCHANGE_RATE = 1.0;

  StockAccountingMethod() = default;

  virtual void apply_trading_day(Book_ptr &book, int64_t trading_day) override {

    auto apply = [&](PositionMap &positions) {
      for (auto &pair : positions) {
        auto &position = pair.second;
        [[maybe_unused]] auto margin_pre = position.margin;
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
      auto &asset_margin = book->asset_margin;

      if (position.direction == Direction::Long) {
        // position.margin would not be changed for Long direction, the margin depends on debt.
        // TODO: As non-margin position and margin position are combined together, need distinguish each volume.
        // asset_margin.margin_market_value += price_change * position.margin_volume;

        asset.market_value += market_value_change; // Asset.market_value means Long positions only.
        asset.unrealized_pnl += market_value_change;
        asset_margin.total_asset += market_value_change;
      } else {
        // short_margin_ratio as 100% when last_price > avg_open_price;
        double short_margin_change = (quote.last_price < position.avg_open_price)
                                         ? cd_mr.short_margin_ratio * market_value_change
                                         : market_value_change;

        position.margin += short_margin_change;
        asset_margin.short_margin += short_margin_change;
        asset_margin.short_market_value += market_value_change;
        // Asset_margin.margin is combined with long_margin and short_margin.
        asset_margin.margin += short_margin_change;
        double avail_margin_change = (price_change && position.direction == Direction::Short)
                                         ? (-cd_mr.conversion_rate * market_value_change - short_margin_change)
                                         : 0;
        asset_margin.avail_margin += avail_margin_change;
        asset.unrealized_pnl -= market_value_change;
      }

      update_position(book, position);
    };
    apply(book->get_position_for(Direction::Long, quote));
    apply(book->get_position_for(Direction::Short, quote));
  }

  virtual void apply_order_input(uint32_t source, uint32_t dest, Book_ptr &book, const OrderInput &input) override {
    if (dest == location::SYNC or dest == location::PUBLIC) {
      return;
    }

    auto &position = book->get_position_for(input);
    auto cd_mr = get_instr_conversion_margin_rate(book, position);
    // double frozen_cash = 0;
    // double frozen_margin = 0;
    double frozen_fee = 0;
    // const auto &asset = book->asset;
    // const auto &asset_margin = book->asset_margin;
    // Offset: Close
    if (input.side == Side::Sell || input.side == Side::RepayMargin) {
      position.frozen_total += input.volume;
      if (position.yesterday_volume - position.frozen_yesterday >= input.volume) {
        position.frozen_yesterday += input.volume;
      } else {
        position.frozen_yesterday = position.yesterday_volume;
      }
    } else if (input.side == Side::Buy) { // Offset: Open
      // TODO: book->asset.frozen_fee += frozen_cash * fee_ratio;

      double frozen_cash = input.volume * input.frozen_price * cd_mr.exchange_rate + frozen_fee;
      book->asset.frozen_cash += frozen_cash;
      book->asset.avail -= frozen_cash;
    } else if (input.side == Side::RepayStock) { // Offset: Close
      // TODO: book->asset.frozen_fee += frozen_cash * fee_ratio;
      double frozen_cash = input.volume * input.frozen_price * cd_mr.exchange_rate + frozen_fee;
      book->asset.frozen_cash += frozen_cash;
      book->asset.avail -= frozen_cash;
      // Short position need frozen
      position.frozen_total += input.volume;
      if (position.yesterday_volume - position.frozen_yesterday >= input.volume) {
        position.frozen_yesterday += input.volume;
      } else {
        position.frozen_yesterday = position.yesterday_volume;
      }
    } else if (input.side == Side::MarginTrade || input.side == Side::ShortSell) {
      // TODO: book->asset.frozen_fee += frozen_cash * fee_ratio;
      double frozen_cash = input.volume * input.frozen_price * cd_mr.exchange_rate + frozen_fee;

      double frozen_margin =
          frozen_cash * (input.side == Side::MarginTrade ? cd_mr.long_margin_ratio : cd_mr.short_margin_ratio);
      book->asset.frozen_margin += frozen_margin;
      book->asset_margin.avail_margin -= frozen_margin;
      if (input.side == Side::MarginTrade) {
        book->asset_margin.cash_margin += frozen_margin;
      } else {
        book->asset_margin.short_margin += frozen_margin;
      }
      book->asset_margin.margin += frozen_margin;
    }
  }

  virtual void apply_order(uint32_t source, uint32_t dest, Book_ptr &book, const Order &order) override {
    if (dest == location::SYNC or dest == location::PUBLIC) {
      return;
    }

    if (not is_final_status(order.status)) {
      return;
    }

    auto &position = book->get_position_for(order);
    auto cd_mr = get_instr_conversion_margin_rate(book, position);

    if (order.side == Side::Buy) {
      auto frozen = book->get_frozen_price(order.order_id) * order.volume_left * cd_mr.exchange_rate;
      book->asset.frozen_cash -= frozen;
      book->asset.avail += frozen;
    } else if (order.side == Side::Sell || order.side == Side::RepayMargin || order.side == Side::RepayStock) {
      position.frozen_total = std::max(position.frozen_total - order.volume_left, VOLUME_ZERO);
      position.frozen_yesterday = std::max(position.frozen_yesterday - order.volume_left, VOLUME_ZERO);
    }

    update_position(book, position);
  }

  virtual void apply_trade(uint32_t source, uint32_t dest, Book_ptr &book, const Trade &trade) override {

    if (trade.side == Side::Sell) {
      apply_sell(dest, book, trade);
    } else if (trade.side == Side::Buy) {
      apply_buy(dest, book, trade);
    }

    auto is_local = dest != location::PUBLIC and dest != location::SYNC;
    if (not is_local) {
      return;
    }

    if (trade.side == Side::MarginTrade) {
      apply_margintrade(book, trade);
    } else if (trade.side == Side::ShortSell) {
      apply_shortsell(book, trade);
    } else if (trade.side == Side::RepayMargin) {
      apply_repaymargin(book, trade);
    } else if (trade.side == Side::RepayStock) {
      apply_repaystock(book, trade);
    }
  }

  virtual void update_position(Book_ptr &book, Position &position) override {
    // auto cd_mr = get_instr_conversion_margin_rate(book, position);
    if (position.last_price > 0) {
      double price_change = position.last_price - position.avg_open_price;
      position.unrealized_pnl =
          (position.direction == Direction::Long ? price_change : -price_change) * position.volume;
      position.update_time = yijinjing::time::now_in_nano();
    }
  }

protected:
  virtual void apply_sell(uint32_t dest, Book_ptr &book, const Trade &trade) {
    auto is_local = dest != location::PUBLIC and dest != location::SYNC;
    auto &position = book->get_position_for(trade);
    auto cd_mr = get_instr_conversion_margin_rate(book, position);
    auto &asset_margin = book->asset_margin;
    double commission = calculate_commission(trade);
    double tax = calculate_tax(trade);
    if (is_local) {
      position.frozen_total = std::max(position.frozen_total - trade.volume, VOLUME_ZERO);
      position.frozen_yesterday = std::max(position.frozen_yesterday - trade.volume, VOLUME_ZERO);
    }
    position.yesterday_volume = std::max(position.yesterday_volume - trade.volume, VOLUME_ZERO);
    position.volume = std::max(position.volume - trade.volume, VOLUME_ZERO);
    double realized_pnl = (trade.price - position.avg_open_price) * trade.volume;
    position.realized_pnl += realized_pnl;

    double unrealized_pnl_change = (position.last_price - position.avg_open_price) * trade.volume;
    position.unrealized_pnl -= unrealized_pnl_change;

    update_position(book, position);

    if (not is_local) {
      return;
    }

    auto &asset = book->asset;
    double trade_amt = trade.price * cd_mr.exchange_rate * trade.volume;

    asset.realized_pnl += realized_pnl * cd_mr.exchange_rate;

    double repay_cash_debt = std::min(position.margin, (trade_amt - (commission + tax)));
    double cash_delivery = trade_amt - repay_cash_debt - (commission + tax);

    asset.avail += cash_delivery;
    asset.intraday_fee += commission + tax;
    asset.accumulated_fee += commission + tax;
  }

  virtual void apply_buy(uint32_t dest, Book_ptr &book, const Trade &trade) {
    auto is_local = dest != location::PUBLIC and dest != location::SYNC;
    auto &position = book->get_position_for(trade);
    auto cd_mr = get_instr_conversion_margin_rate(book, position);
    double trade_amt = trade.price * trade.volume * cd_mr.exchange_rate;
    auto &asset_margin = book->asset_margin;
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
    position.volume += trade.volume;
    update_position(book, position);

    if (not is_local) {
      return;
    }

    auto &asset = book->asset;
    asset.intraday_fee += commission + tax;
    asset.accumulated_fee += commission + tax;
  }

  virtual void apply_margintrade(Book_ptr &book, const Trade &trade) {
    auto &position = book->get_position_for(trade);

    auto cd_mr = get_instr_conversion_margin_rate(book, position);
    double trade_amt = trade.price /** cd_mr.exchange_rate*/ * trade.volume;
    // TODO: margin_commission requires a dedicate calculate_margin_commission(Trade&);
    auto &asset_margin = book->asset_margin;
    double commission = calculate_commission(trade);
    auto tax = calculate_tax(trade);
    if (not position.last_price) {
      position.last_price = trade.price;
    }

    // The market_value calculation would be reset in Book::update(), so it does not matter if has
    // deviation of the actual status.
    // TODO: the commission & tax should be included ?
    double cash_debt_change = trade_amt;
    double original_volume = position.volume;
    if (position.volume + trade.volume > 0 && trade.price > 0) {
      position.avg_open_price =
          (position.avg_open_price * original_volume + trade_amt) / (double)(original_volume + trade.volume);
      position.position_cost_price = (position.position_cost_price * position.volume + trade_amt + commission + tax) /
                                     (double)(position.volume + trade.volume);
    }
    // Track the MarginTrade part cash_debt
    position.margin += cash_debt_change; // The margin is actually the cash debt of Position instead of margin
    position.volume += trade.volume;
    update_position(book, position);
  }

  virtual void apply_shortsell(Book_ptr &book, const Trade &trade) {
    auto &position = book->get_position_for(trade);
    auto cd_mr = get_instr_conversion_margin_rate(book, position);
    double trade_amt = trade.price * trade.volume * cd_mr.exchange_rate;
    // TODO: margin_commission requires a dedicate calculate_margin_commission(Trade&);
    auto &asset_margin = book->asset_margin;
    double commission = calculate_commission(trade);
    auto tax = calculate_tax(trade);
    if (position.volume + trade.volume > 0 && trade.price > 0) {
      position.avg_open_price = (position.avg_open_price * position.volume + trade_amt / cd_mr.exchange_rate) /
                                (double)(position.volume + trade.volume);
      position.position_cost_price =
          (position.position_cost_price * position.volume + trade_amt / cd_mr.exchange_rate - commission - tax) /
          (double)(position.volume + trade.volume);
    }
    double original_volume = position.volume;
    position.volume += trade.volume;
    // The market value is calculated in Book::update()
    if (not position.last_price) {
      position.last_price = trade.price;
    }
    position.last_price = position.last_price > 0 ? position.last_price : position.avg_open_price;
    update_position(book, position);
  }

  virtual void apply_repaymargin(Book_ptr &book, const Trade &trade) {
    auto &position = book->get_position_for(trade);
    auto cd_mr = get_instr_conversion_margin_rate(book, position);
    if (not position.last_price) {
      position.last_price = trade.price;
    }
    auto &asset_margin = book->asset_margin;
    double commission = calculate_commission(trade);
    auto tax = calculate_tax(trade);
    position.frozen_total = std::max(position.frozen_total - trade.volume, VOLUME_ZERO);
    position.frozen_yesterday = std::max(position.frozen_yesterday - trade.volume, VOLUME_ZERO);
    position.yesterday_volume = std::max(position.yesterday_volume - trade.volume, VOLUME_ZERO);
    position.volume = std::max(position.volume - trade.volume, VOLUME_ZERO);
    position.realized_pnl += (trade.price - position.avg_open_price) * trade.volume;
    update_position(book, position);
  }

  virtual void apply_repaystock(Book_ptr &book, const Trade &trade) {

    auto &position = book->get_position_for(trade);
    auto cd_mr = get_instr_conversion_margin_rate(book, position);
    auto &asset_margin = book->asset_margin;
    double commission = calculate_commission(trade);
    auto tax = calculate_tax(trade);
    // Position Direction: Short
    position.frozen_total = std::max(position.frozen_total - trade.volume, VOLUME_ZERO);
    position.frozen_yesterday = std::max(position.frozen_yesterday - trade.volume, VOLUME_ZERO);
    position.yesterday_volume = std::max(position.yesterday_volume - trade.volume, VOLUME_ZERO);
    position.volume = std::max(position.volume - trade.volume, VOLUME_ZERO);

    position.last_price = position.last_price > 0 ? position.last_price : trade.price;

    double repay_debt_mrkt_value = position.last_price * cd_mr.exchange_rate * trade.volume;
    double released_margin = repay_debt_mrkt_value * cd_mr.short_margin_ratio;

    auto realized_pnl = (position.avg_open_price - trade.price) * trade.volume;
    position.realized_pnl += realized_pnl;
    position.margin -= released_margin;
    update_position(book, position);
  }

  virtual double calculate_commission(const Trade &trade) { return trade.commission; }

  virtual double calculate_tax(const Trade &trade) { return trade.tax; }

  static contract_discount_and_margin_ratio get_instr_conversion_margin_rate(const Book_ptr &book,
                                                                             const Position &position) {
    const char *exchange_id = position.exchange_id;
    const char *instrument_id = position.instrument_id;
    // SPDLOG_TRACE("position exchange_id {} instrument_id {} ", exchange_id, instrument_id);
    uint32_t hashed_instrument_key = hash_instrument(exchange_id, instrument_id);
    contract_discount_and_margin_ratio cd_mr = {};

    try {
      if (book->instruments.find(hashed_instrument_key) == book->instruments.end()) {
        cd_mr.contract_multiplier = DEFAULT_STOCK_CONTRACT_MULTIPLIER;
      } else {
        const auto &instrument = book->instruments.at(hashed_instrument_key);
        cd_mr.contract_multiplier = instrument.contract_multiplier;
      }

      if (book->instrument_factors.find(hashed_instrument_key) == book->instrument_factors.end()) {
        cd_mr.margin_ratio =
            position.direction == Direction::Long ? DEFAULT_STOCK_LONG_MARGIN_RATIO : DEFAULT_STOCK_SHORT_MARGIN_RATIO;
        cd_mr.long_margin_ratio = DEFAULT_STOCK_LONG_MARGIN_RATIO;
        cd_mr.short_margin_ratio = DEFAULT_STOCK_SHORT_MARGIN_RATIO;
        cd_mr.conversion_rate = DEFAULT_STOCK_CONVERSION_RATE;
        cd_mr.exchange_rate = DEFAULT_STOCK_EXCHANGE_RATE;
      } else {
        auto &factor = book->instrument_factors.at(hashed_instrument_key);
        cd_mr.margin_ratio = margin_ratio(factor, position);
        cd_mr.long_margin_ratio = factor.long_margin_ratio;
        cd_mr.short_margin_ratio = factor.short_margin_ratio;
        cd_mr.conversion_rate = factor.conversion_rate;
        cd_mr.exchange_rate = is_equal(factor.exchange_rate, 0.0) ? 1.0 : factor.exchange_rate;
      }
    } catch (std::exception &ex) {
      SPDLOG_ERROR("Exception for instrument_id {}: {}", instrument_id, ex.what());
      cd_mr.contract_multiplier = DEFAULT_STOCK_CONTRACT_MULTIPLIER;
      cd_mr.margin_ratio =
          position.direction == Direction::Long ? DEFAULT_STOCK_LONG_MARGIN_RATIO : DEFAULT_STOCK_SHORT_MARGIN_RATIO;
      cd_mr.long_margin_ratio = DEFAULT_STOCK_LONG_MARGIN_RATIO;
      cd_mr.short_margin_ratio = DEFAULT_STOCK_SHORT_MARGIN_RATIO;
      cd_mr.conversion_rate = DEFAULT_STOCK_CONVERSION_RATE;
      cd_mr.exchange_rate = DEFAULT_STOCK_EXCHANGE_RATE;
    }
    return cd_mr;
  }

  static double margin_ratio(const InstrumentFactor &factor, const Position &position) {
    return position.direction == Direction::Long ? factor.long_margin_ratio : factor.short_margin_ratio;
  }
  [[maybe_unused]] static double roundn(double value, int n = AMOUT_PRECISION) {
    double x = pow(10.0, (double)n);
    double round_val = round(value * x) / x;
    return round_val;
  }
};
} // namespace kungfu::wingchun::book
#endif // WINGCHUN_ACCOUNTING_STOCK_H
