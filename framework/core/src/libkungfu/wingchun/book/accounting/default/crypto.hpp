// SPDX-License-Identifier: Apache-2.0

//
// Created by qianyong liu on 2021/7/27.
//

#ifndef WINGCHUN_ACCOUNTING_CRYPTO_H
#define WINGCHUN_ACCOUNTING_CRYPTO_H

#include <kungfu/wingchun/book/accounting.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::wingchun;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;

namespace kungfu::wingchun::book {
class CryptoAccountingMethod : public AccountingMethod {
public:
  void get_instrument(const Book_ptr &book, const Trade &trade, char *instrument_a, char *instrument_b,
                      char *instrument_commission, int64_t &volume_a, int64_t &volume_b, int64_t &volume_commission) {
    auto instrument_hash = hash_instrument(trade.exchange_id, trade.instrument_id);
    const auto &instrument = book->instruments.at(instrument_hash);
    strncpy(instrument_a, instrument.instrument_id, strlen(instrument.instrument_id) - instrument.delivery_year);
    strcpy(instrument_b, &(instrument.instrument_id[strlen(instrument.instrument_id) - instrument.delivery_year]));
    if (trade.hedge_flag == HedgeFlag::Speculation)
      strcpy(instrument_commission, "BNB");
    else if (trade.hedge_flag == HedgeFlag::Arbitrage)
      strcpy(instrument_commission, instrument_b);
    else
      strcpy(instrument_commission, instrument_a);
    volume_a = trade.volume;
    volume_b = trade.volume * trade.price;
    volume_commission = trade.commission;
  }

  CryptoAccountingMethod() = default;

  virtual void apply_quote(Book_ptr &book, const Quote &quote) override {}

  virtual void apply_order_input(uint32_t account_id, uint32_t dest, Book_ptr &book, const OrderInput &input) override {
  }

  virtual void apply_order(uint32_t account_id, uint32_t dest, Book_ptr &book, const Order &order) override {
    if (not guard_order_accounting(account_id, dest, book, order)) {
      return;
    }
  }

  virtual void apply_trade(uint32_t account_id, uint32_t dest, Book_ptr &book, const Trade &trade) override {
    if (not guard_trade_accounting(account_id, dest, book, trade)) {
      return;
    }
  }

  virtual void update_position(Book_ptr &book, Position &position) override {}

  bool update_asset(const map::InstrumentMap &instruments, const map::InstrumentFactorMap &instrument_factors,
                    Asset &asset, const Position &position) override {
    return false;
  }

protected:
  std::unordered_map<uint64_t, double> commission_map_ = {};

  [[maybe_unused]] virtual void apply_buy(Book_ptr &book, const Trade &trade) {}

  [[maybe_unused]] virtual void apply_sell(Book_ptr &book, const Trade &trade) {}

  [[maybe_unused]] double calculate_commission(const Trade &trade) { return trade.commission; }

  [[maybe_unused]] double calculate_tax(const Trade &trade) { return trade.tax; }
};
} // namespace kungfu::wingchun::book
#endif // WINGCHUN_ACCOUNTING_CRYPTO_H