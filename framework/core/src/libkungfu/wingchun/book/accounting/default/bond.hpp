// SPDX-License-Identifier: Apache-2.0

#ifndef WINGCHUN_ACCOUNTING_BOND_H
#define WINGCHUN_ACCOUNTING_BOND_H

#include "stock.hpp"

namespace kungfu::wingchun::book {

class BondAccountingMethod : public StockAccountingMethod {
public:
  BondAccountingMethod() = default;

  void apply_order_input(uint32_t account_id, uint32_t dest, Book_ptr &book, const OrderInput &input) override {
    if (dest == location::SYNC or dest == location::PUBLIC) {
      return;
    }

    auto apply = [&](auto &position) {
      auto cd_mr = get_instrument_conversion_margin_rate(book, position.source_id, position.direction,
                                                         position.exchange_id, position.instrument_id);
      if (!is_convertible_bond(input.instrument_id, input.exchange_id)) {
        StockAccountingMethod::apply_order_input(account_id, dest, book, input);
        return;
      }

      if (input.side == Side::Sell && position.volume - position.frozen_total >= input.volume) {
        position.frozen_total += input.volume;
        position.frozen_yesterday = std::max(position.frozen_yesterday + input.volume, position.yesterday_volume);
      }
      if (input.side == Side::Buy) {
        book->asset.frozen_cash += input.volume * input.frozen_price * cd_mr.exchange_rate;
        book->asset.avail -= input.volume * input.frozen_price * cd_mr.exchange_rate;
      }
    };

    book->apply_position_for(account_id, input, apply);
  }
};
} // namespace kungfu::wingchun::book
#endif