//
// Created by marsjliu on 2023/4/11.
//

#ifndef WINGCHUN_ACCOUNTING_FUTURE_LONG_SHORT_H
#define WINGCHUN_ACCOUNTING_FUTURE_LONG_SHORT_H

#include <kungfu/wingchun/book/accounting.h>
#include <kungfu/wingchun/book/bookkeeper.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;

namespace kungfu::wingchun::book {

#define OTC_DEFAULT_INSTRUMENT_CONTRACT_MULTIPLIER 10
#define OTC_DEFAULT_INSTRUMENT_LONG_MARGIN_RATIO 0.1
#define OTC_DEFAULT_INSTRUMENT_SHORT_MARGIN_RATIO 0.1
#define OTC_DEFAULT_INSTRUMENT_EXCHANGE_RATE 1.0

struct otc_future_contract_multiplier_and_margin_ratio {
  int32_t contract_multiplier;
  double margin_ratio;
  double exchange_rate;
};

class OtcFutureAccountingMethod : public AccountingMethod {
public:
  OtcFutureAccountingMethod() = default;

  void apply_quote(Book_ptr &book, const Quote &quote) override {
    auto apply = [&](auto &position) {
      auto cm_mr = get_instrument_contract_multiplier_and_margin_ratio(book, position.source_id, position.direction,
                                                                       position.exchange_id, position.instrument_id);

      if (is_valid_price(quote.settlement_price)) {
        auto margin_pre = position.margin;
        position.margin = cm_mr.contract_multiplier * position.settlement_price * cm_mr.exchange_rate *
                          position.volume * cm_mr.margin_ratio;

        position.settlement_price = quote.settlement_price;
        book->asset.avail -= (position.direction == Direction::Long ? 1 : -1) * (position.margin - margin_pre);
      }

      if (is_valid_price(quote.last_price) && is_valid_price(position.last_price)) {
        double price_change = quote.last_price - position.last_price;
        position.last_price = quote.last_price;
        double market_value_change = (position.direction == Direction::Long ? 1 : -1) * price_change *
                                     cm_mr.exchange_rate * position.volume * cm_mr.contract_multiplier;
        book->asset.market_value += market_value_change;
      }

      if (is_valid_price(quote.last_price)) {
        position.last_price = quote.last_price;
      }

      if (is_valid_price(quote.pre_settlement_price)) {
        position.pre_settlement_price = quote.pre_settlement_price;
      }

      update_position(book, position);
    };

    book->apply_long_position_for(quote, apply);
    book->apply_short_position_for(quote, apply);
  }

  void apply_order_input(Book_ptr &book, uint32_t account_id, const OrderInput &input) override {
    auto offset = get_offset(book, account_id, input);
    auto apply = [&](auto &position) {
      auto cm_mr = get_instrument_contract_multiplier_and_margin_ratio(book, account_id, position.direction,
                                                                       input.exchange_id, input.instrument_id);
      if (offset == Offset::Open) {
        auto frozen_margin =
            cm_mr.contract_multiplier * input.frozen_price * cm_mr.exchange_rate * input.volume * cm_mr.margin_ratio;

        book->asset.avail -= frozen_margin;
        book->asset.frozen_cash += frozen_margin;
        book->asset.frozen_margin += frozen_margin;
      }

      if (offset == Offset::Close or offset == Offset::CloseYesterday) {
        position.frozen_total += input.volume;
        if (position.yesterday_volume - position.frozen_yesterday >= input.volume) {
          position.frozen_yesterday += input.volume;
        } else {
          position.frozen_yesterday = position.yesterday_volume;
        }
      }

      if (offset == Offset::CloseToday) {
        position.frozen_total += input.volume;
      }

      update_position(book, position);
    };

    auto direction = get_direction(input.instrument_type, input.side, offset);
    book->apply_position(account_id, direction, input.exchange_id, input.instrument_id, apply);
  }

  void apply_order(Book_ptr &book, uint32_t account_id, const Order &order) override {
    if (not is_final_status(order.status))
      return;

    auto offset = get_offset(book, account_id, order);
    auto apply = [&](auto &position) {
      auto cm_mr = get_instrument_contract_multiplier_and_margin_ratio(book, account_id, position.direction,
                                                                       position.exchange_id, position.instrument_id);
      if (offset == Offset::Open) {
        auto frozen_margin = cm_mr.contract_multiplier * order.frozen_price * cm_mr.exchange_rate * order.volume_left *
                             cm_mr.margin_ratio;
        book->asset.avail += frozen_margin;
        book->asset.frozen_cash -= frozen_margin;
        book->asset.frozen_margin -= frozen_margin;

        auto frozen_market_value =
            cm_mr.contract_multiplier * order.frozen_price * cm_mr.exchange_rate * order.volume_left;
        book->asset.market_value += frozen_market_value;
      }

      if (offset == Offset::Close or offset == Offset::CloseYesterday) {
        position.frozen_total = std::max(position.frozen_total - order.volume_left, VOLUME_ZERO);
        position.frozen_yesterday = std::max(position.frozen_yesterday - order.volume_left, VOLUME_ZERO);
      }

      if (offset == Offset::CloseToday and position.frozen_total >= order.volume_left) {
        position.frozen_total -= order.volume_left;
      }

      update_position(book, position);
    };

    auto direction = get_direction(order.instrument_type, order.side, offset);
    book->apply_position(account_id, direction, order.exchange_id, order.instrument_id, apply);
  }

  void apply_trade(Book_ptr &book, uint32_t account_id, const Trade &trade) override {
    auto offset = get_offset(book, account_id, trade);
    auto apply = [&](auto &position) {
      if (offset == Offset::Open) {
        apply_open(book, position, trade);
      }
      if (offset == Offset::Close or offset == Offset::CloseToday or offset == Offset::CloseYesterday) {
        apply_close(book, position, trade);
      }
    };

    auto direction = get_direction(trade.instrument_type, trade.side, offset);
    book->apply_position(account_id, direction, trade.exchange_id, trade.instrument_id, apply);
  }

  void update_position(Book_ptr &book, Position &position) override {
    if (position.last_price > 0) {
      auto cm_mr = get_instrument_contract_multiplier_and_margin_ratio(book, position.source_id, position.direction,
                                                                       position.exchange_id, position.instrument_id);
      uint32_t product_key = hash_product(position.exchange_id, get_instrument_product(position.product_id).c_str());
      double cost = 0;

      if (book->commissions.find(product_key) != book->commissions.end()) {
        const auto &commission = book->commissions.at(product_key);
        auto close_today_volume = double(position.volume - position.yesterday_volume);
        if (commission.mode == CommissionRateMode::ByAmount) {
          cost = (position.last_price * position.yesterday_volume * commission.close_ratio) +
                 (position.last_price * close_today_volume * commission.close_today_ratio);

          cost = cost * cm_mr.contract_multiplier;
        } else {
          // by volume calculate
          cost = (position.yesterday_volume * commission.close_ratio) +
                 (close_today_volume * commission.close_today_ratio);
        }
      }

      auto multiplier = cm_mr.contract_multiplier * (position.direction == Direction::Long ? 1 : -1);
      auto price_diff = position.last_price - position.avg_open_price;
      position.unrealized_pnl = (price_diff * position.volume) * multiplier - cost;
    }
  }

private:
  void apply_open(Book_ptr &book, Position &position, const Trade &trade) {
    auto cm_mr = get_instrument_contract_multiplier_and_margin_ratio(book, position.source_id, position.direction,
                                                                     trade.exchange_id, trade.instrument_id);

    auto contract_multiplier = cm_mr.contract_multiplier;
    auto margin_ratio_by_pos = cm_mr.margin_ratio;
    auto margin = contract_multiplier * trade.price * cm_mr.exchange_rate * trade.volume * margin_ratio_by_pos;
    auto frozen_margin = contract_multiplier * book->get_frozen_price(trade.order_id) * cm_mr.exchange_rate *
                         trade.volume * margin_ratio_by_pos;
    position.margin += margin;
    position.avg_open_price = (position.avg_open_price * position.volume + trade.price * trade.volume) /
                              double(position.volume + trade.volume);
    position.volume += trade.volume;
    update_position(book, position);

    book->asset.avail += frozen_margin;
    book->asset.frozen_cash -= frozen_margin;
    book->asset.frozen_margin -= frozen_margin;
    auto trade_market_value = contract_multiplier * position.last_price * cm_mr.exchange_rate * trade.volume;
    book->asset.market_value += trade_market_value;

    auto commission = calculate_commission(book, trade, position, 0) * cm_mr.exchange_rate;
    book->asset.avail -= commission;
    book->asset.avail -= margin;
    book->asset.accumulated_fee += commission;
    book->asset.intraday_fee += commission;
    book->asset.margin += margin;
  }

  void apply_close(Book_ptr &book, Position &position, const Trade &trade) {
    auto cm_mr = get_instrument_contract_multiplier_and_margin_ratio(book, position.source_id, position.direction,
                                                                     trade.exchange_id, trade.instrument_id);
    auto contract_multiplier = cm_mr.contract_multiplier;
    auto margin = contract_multiplier * trade.price * cm_mr.exchange_rate * trade.volume * cm_mr.margin_ratio;
    auto delta_margin = std::min(position.margin, margin);
    position.margin -= delta_margin;
    position.volume -= trade.volume;
    position.frozen_total -= trade.volume;
    auto close_today_volume = 0.0;
    if (trade.offset != Offset::CloseToday) {
      close_today_volume = std::max(trade.volume - position.yesterday_volume, VOLUME_ZERO);
      position.yesterday_volume = std::max(position.yesterday_volume - trade.volume, VOLUME_ZERO);
      position.frozen_yesterday = std::max(position.frozen_yesterday - trade.volume, VOLUME_ZERO);
    } else {
      close_today_volume = trade.volume;
    }

    auto realized_pnl = (trade.price - position.avg_open_price) * trade.volume * contract_multiplier;
    if (position.direction == Direction::Short) {
      realized_pnl = -realized_pnl;
    }
    position.realized_pnl += realized_pnl;
    update_position(book, position);

    auto trade_market_value = contract_multiplier * trade.price * cm_mr.exchange_rate * trade.volume;
    book->asset.market_value -= trade_market_value;

    auto commission = calculate_commission(book, trade, position, close_today_volume) * cm_mr.exchange_rate;
    book->asset.realized_pnl += realized_pnl * cm_mr.exchange_rate;
    book->asset.avail += delta_margin;
    book->asset.avail -= commission;
    book->asset.accumulated_fee += commission;
    book->asset.intraday_fee += commission;
  }

  template <typename TradingData>
  [[nodiscard]] bool need_to_merge_long_short_positions(Book_ptr &book, uint32_t account_id,
                                                        const TradingData &trading_data) const {
    if (not able_long_short_position_merge(trading_data.exchange_id))
      return false;

    auto flag = false;
    auto apply = [&](auto &position) {
      if (position.volume > 0) {
        flag = true;
      }
    };
    book->apply_opposite_position_for(account_id, trading_data, apply);
    return flag;
  }

  template <typename TradingData>
  [[nodiscard]] bool need_to_open_opposite(Book_ptr &book, uint32_t account_id, const TradingData &trading_data) const {
    if (not able_long_short_position_merge(trading_data.exchange_id))
      return false;

    auto flag = false;
    auto apply = [&](auto &position) {
      if (position.volume <= 0 && trading_data.offset != Offset::Open) {
        flag = true;
      }
    };

    book->apply_position_for(account_id, trading_data, apply);
    return flag;
  }

  template <typename TradingData>
  [[nodiscard]] longfist::enums::Offset get_offset(Book_ptr &book, uint32_t account_id,
                                                   const TradingData &trading_data) const {
    auto offset = trading_data.offset;
    if (need_to_merge_long_short_positions(book, account_id, trading_data) && offset == Offset::Open) {
      return Offset::Close;
    }

    if (need_to_open_opposite(book, account_id, trading_data) && offset != Offset::Open) {
      return Offset::Open;
    }

    return offset;
  }

  static double calculate_commission(Book_ptr &book, const Trade &trade, const Position &position,
                                     double close_today_volume) {
    auto cm_mr = get_instrument_contract_multiplier_and_margin_ratio(book, position.source_id, position.direction,
                                                                     trade.exchange_id, trade.instrument_id);

    auto contract_multiplier = cm_mr.contract_multiplier;
    uint32_t product_key = hash_product(trade.exchange_id, get_instrument_product(trade.product_id).c_str());
    if (book->commissions.find(product_key) == book->commissions.end()) {
      SPDLOG_WARN("commission information missing for {}@{}", trade.instrument_id, trade.exchange_id);
      return 0;
    }
    const auto &commission = book->commissions.at(product_key);
    if (commission.mode == CommissionRateMode::ByAmount) {
      if (trade.offset == Offset::Open) {
        return trade.price * cm_mr.exchange_rate * trade.volume * contract_multiplier * commission.open_ratio;
      } else {
        auto volume_left = double(trade.volume) - close_today_volume;
        return (trade.price * volume_left * contract_multiplier * commission.close_ratio) +
               (trade.price * close_today_volume * contract_multiplier * commission.close_today_ratio);
      }
    } else {
      if (trade.offset == Offset::Open) {
        return double(trade.volume) * commission.open_ratio;
      } else {
        auto volume_left = double(trade.volume - close_today_volume);
        return (volume_left * commission.close_ratio) + (close_today_volume * commission.close_today_ratio);
      }
    }
  }

  static otc_future_contract_multiplier_and_margin_ratio
  get_instrument_contract_multiplier_and_margin_ratio(Book_ptr &book, uint32_t account_id,
                                                      longfist::enums::Direction direction, const char *exchange_id,
                                                      const char *instrument_id) {
    auto hashed_instrument_key = hash_instrument(exchange_id, instrument_id);
    otc_future_contract_multiplier_and_margin_ratio cm_mr = {};
    if (book->instruments.find(hashed_instrument_key) == book->instruments.end()) {
      SPDLOG_WARN("instrument information missing for {}@{}", instrument_id, exchange_id);
      cm_mr.contract_multiplier = OTC_DEFAULT_INSTRUMENT_CONTRACT_MULTIPLIER;
    } else {
      const auto &instrument = book->instruments.at(hashed_instrument_key);
      cm_mr.contract_multiplier = instrument.contract_multiplier;
    }

    auto hashed_instrument_factor_key = hash_instrument(account_id, exchange_id, instrument_id);
    if (book->instrument_factors.find(hashed_instrument_factor_key) == book->instrument_factors.end()) {
      cm_mr.exchange_rate = OTC_DEFAULT_INSTRUMENT_EXCHANGE_RATE;
      cm_mr.margin_ratio = direction == Direction::Long ? OTC_DEFAULT_INSTRUMENT_LONG_MARGIN_RATIO
                                                        : OTC_DEFAULT_INSTRUMENT_SHORT_MARGIN_RATIO;
    } else {
      auto &factor = book->instrument_factors.at(hashed_instrument_factor_key);
      cm_mr.margin_ratio = direction == Direction::Long ? factor.long_margin_ratio : factor.short_margin_ratio;
      cm_mr.exchange_rate = is_equal(factor.exchange_rate, 0.0) ? 1.0 : factor.exchange_rate;
    }
    return cm_mr;
  }

  static bool able_long_short_position_merge(const char *exchange_id) {
    if (strcmp(exchange_id, EXCHANGE_US_FUTURE) == 0 || strcmp(exchange_id, EXCHANGE_HK_FUTURE) == 0 ||
        strcmp(exchange_id, EXCHANGE_SGX_FUTURE) == 0 || strcmp(exchange_id, EXCHANGE_LON_FUTURE) == 0 ||
        strcmp(exchange_id, EXCHANGE_AEX_FUTURE) == 0 || strcmp(exchange_id, EXCHANGE_AUX_FUTURE) == 0 ||
        strcmp(exchange_id, EXCHANGE_HEXS_FUTURE) == 0 || strcmp(exchange_id, EXCHANGE_IDX_FUTURE) == 0 ||
        strcmp(exchange_id, EXCHANGE_KORC) == 0 || strcmp(exchange_id, EXCHANGE_LME) == 0 ||
        strcmp(exchange_id, EXCHANGE_MYS_FUTURE) == 0 || strcmp(exchange_id, EXCHANGE_ABB) == 0 ||
        strcmp(exchange_id, EXCHANGE_PRX_FUTURE) == 0 || strcmp(exchange_id, EXCHANGE_SIX_FUTURE) == 0 ||
        strcmp(exchange_id, EXCHANGE_TAX_FUTURE) == 0 || strcmp(exchange_id, EXCHANGE_JP_FUTURE) == 0 ||
        strcmp(exchange_id, EXCHANGE_TSE_FUTURE) == 0 || strcmp(exchange_id, EXCHANGE_XETRA) == 0 ||
        strcmp(exchange_id, EXCHANGE_EUR_FUTURE) == 0) {
      return true;
    }

    return false;
  }
};
} // namespace kungfu::wingchun::book
#endif // WINGCHUN_ACCOUNTING_FUTURE_LONG_SHORT_H
