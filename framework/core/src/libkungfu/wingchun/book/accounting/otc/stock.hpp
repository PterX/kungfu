// SPDX-License-Identifier: Apache-2.0

//
// Created by marsjliu on 2023/4/11.
//

#ifndef WINGCHUN_ACCOUNTING_STOCK_LONG_SHORT_H
#define WINGCHUN_ACCOUNTING_STOCK_LONG_SHORT_H

#include "../default/stock.hpp"
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

class OtcStockAccountingMethod : public StockAccountingMethod {
public:
  OtcStockAccountingMethod() = default;

  void apply_trade(uint32_t source, uint32_t dest, Book_ptr &book, const Trade &trade) override {
    if (not guard_trade_accounting(book, trade)) {
      return;
    }
    
    auto is_local = dest != location::PUBLIC and dest != location::SYNC;

    if (trade.side == Side::Sell) {
      apply_sell(book, trade, is_local);
    } else if (trade.side == Side::Buy) {
      apply_buy(book, trade, is_local);
    }
  }
};
} // namespace kungfu::wingchun::book
#endif // WINGCHUN_ACCOUNTING_STOCK_LONG_SHORT_H
