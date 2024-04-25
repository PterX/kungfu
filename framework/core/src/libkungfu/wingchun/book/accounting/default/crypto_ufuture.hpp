// SPDX-License-Identifier: Apache-2.0

//
// Created by marsjliu on 2024/3/28.
//

#ifndef WINGCHUN_ACCOUNTING_CRYPTO_UFUTURE_H
#define WINGCHUN_ACCOUNTING_CRYPTO_UFUTURE_H

#include <kungfu/wingchun/book/accounting.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;

namespace kungfu::wingchun::book {
struct cryptoufuture_attribute {
  int32_t contract_multiplier;
  double contract_val = 1.0; // 合约面值
};

class CryptoUFutureAccountingMethod : public AccountingMethod {
public:
  CryptoUFutureAccountingMethod() = default;

  virtual void apply_quote(Book_ptr &book, const Quote &quote) override {}

  void apply_order_input(uint32_t account_id, uint32_t dest, Book_ptr &book, const OrderInput &input) override {
    if (dest == location::SYNC or dest == location::PUBLIC) {
      return;
    }

    auto apply = [&](auto &position) {
      auto crypto_uf_a = get_crypto_ufut_instrument_attribute(book, position.source_id, position.direction,
                                                              input.exchange_id, input.instrument_id);
      if (input.offset != Offset::Open) {
        position.frozen_total += input.volume;
      }

      update_position(book, position);
    };

    auto direction = get_direction(input.instrument_type, input.side, input.offset);
    book->apply_position(account_id, direction, input.exchange_id, input.instrument_id, apply);
  }

  void apply_order(uint32_t account_id, uint32_t dest, Book_ptr &book, const Order &order) override {
    if (not guard_order_accounting(account_id, dest, book, order)) {
      return;
    }

    auto apply = [&](auto &position) {
      auto crypto_uf_a = get_crypto_ufut_instrument_attribute(book, position.source_id, position.direction,
                                                              order.exchange_id, order.instrument_id);
      if (order.offset != Offset::Open) {
        position.frozen_total = std::max(position.frozen_total - order.volume_left, VOLUME_ZERO);
      }

      update_position(book, position);
    };

    auto direction = get_direction(order.instrument_type, order.side, order.offset);
    book->apply_position(account_id, direction, order.exchange_id, order.instrument_id, apply);
  }

  void apply_trade(uint32_t account_id, uint32_t dest, Book_ptr &book, const Trade &trade) override {
    if (not guard_trade_accounting(account_id, dest, book, trade)) {
      return;
    }

    auto is_local = dest != location::PUBLIC and dest != location::SYNC;
    auto offset = trade.offset;
    auto apply = [&](auto &position) {
      if (offset == Offset::Open) {
        apply_open(book, position, trade, is_local);
      }
      if (offset == Offset::Close or offset == Offset::CloseToday or offset == Offset::CloseYesterday) {
        apply_close(book, position, trade, is_local);
      }
    };

    auto direction = get_direction(trade.instrument_type, trade.side, offset);
    SPDLOG_DEBUG("apply_trade ----成交u本位 instrument_id={}, volume = {}, order_id={}, trade_id={}, direction={}",
                 trade.instrument_id, trade.volume, trade.order_id, trade.trade_id, int(direction));

    book->apply_position(account_id, direction, trade.exchange_id, trade.instrument_id, apply);
  }

  void update_position(Book_ptr &book, Position &position) override {
    if (position.last_price <= 0) {
      return;
    }

    // auto crypto_uf_a = get_crypto_ufut_instrument_attribute(book, position.source_id, position.direction,
    //                                                   position.exchange_id, position.instrument_id);
    // auto multiplier = crypto_uf_a.contract_multiplier * (position.direction == Direction::Long ? 1 : -1);
    // auto price_diff = position.last_price - position.avg_open_price;
    // // 浮动盈亏
    // // USDT保证金合约多仓收益 = 面值 * |张数| * 合约乘数 *（标记价格 - 开仓均价）
    // // 空仓收益 = 面值 * |张数| * 合约乘数 *（开仓均价 - 标记价格）
    // position.unrealized_pnl = (price_diff * position.volume) * multiplier;
  }

  bool update_asset(const map::InstrumentMap &instruments, const map::InstrumentFactorMap &instrument_factors,
                    Asset &asset, const Position &position) override {
    return false;
  }

private:
  void apply_open(Book_ptr &book, Position &position, const Trade &trade, bool is_local) {
    if (position.volume + trade.volume > 0 && trade.price > 0) { // only calculate when greater than 0
      position.avg_open_price = (position.avg_open_price * position.volume + trade.price * trade.volume) /
                                double(position.volume + trade.volume);
    }
    position.volume += trade.volume;
    position.open_volume += trade.volume;
    position.last_price = position.last_price > 0 ? position.last_price : trade.price;
    update_position(book, position);
  }

  void apply_close(Book_ptr &book, Position &position, const Trade &trade, bool is_local) {
    position.volume -= trade.volume;
    position.last_price = position.last_price > 0 ? position.last_price : trade.price;

    if (is_local) {
      position.frozen_total = std::max(position.frozen_total - trade.volume, VOLUME_ZERO);
    }
  }

  static cryptoufuture_attribute get_crypto_ufut_instrument_attribute(Book_ptr &book, uint32_t account_id,
                                                                      longfist::enums::Direction direction,
                                                                      const char *exchange_id,
                                                                      const char *instrument_id) {
    return get_crypto_ufut_instrument_attribute(book->instruments, account_id, direction, exchange_id, instrument_id);
  }

  static cryptoufuture_attribute get_crypto_ufut_instrument_attribute(const map::InstrumentMap &instruments,
                                                                      uint32_t account_id,
                                                                      longfist::enums::Direction direction,
                                                                      const char *exchange_id,
                                                                      const char *instrument_id) {

    auto hashed_instrument_key = hash_instrument(exchange_id, instrument_id);
    cryptoufuture_attribute crypto_uf_a = {};
    if (instruments.find(hashed_instrument_key) == instruments.end()) {
      crypto_uf_a.contract_multiplier = DEFAULT_INSTRUMENT_CONTRACT_MULTIPLIER;
    } else {
      const auto &instrument = instruments.at(hashed_instrument_key);
      crypto_uf_a.contract_multiplier = instrument.contract_multiplier;
    }

    // 合约面值
    crypto_uf_a.contract_val = 1.0;
    return crypto_uf_a;
  }
};
} // namespace kungfu::wingchun::book
#endif // WINGCHUN_ACCOUNTING_CRYPTO_UFUTURE_H
