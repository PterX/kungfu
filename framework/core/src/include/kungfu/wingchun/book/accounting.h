// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/4/6.
//

#ifndef WINGCHUN_ACCOUNTING_H
#define WINGCHUN_ACCOUNTING_H

#include <kungfu/longfist/enums.h>
#include <kungfu/wingchun/book/book.h>

using namespace kungfu::longfist::enums;

namespace kungfu::wingchun::book {
static const longfist::enums::AccountingMethodType get_accounting_method_type() {
  char *is_outside = std::getenv("IS_OUTSIDE_ACCOUNTING_TYPE");
  if (is_outside == nullptr) {
    SPDLOG_INFO("AccountingMethod::setup_defaults IS_OUTSIDE_ACCOUNTING_TYPE is unset, use DEFAULT");
    return longfist::enums::AccountingMethodType::Default;
  }

  SPDLOG_INFO("AccountingMethod::setup_defaults IS_OUTSIDE_ACCOUNTING_TYPE = {}", is_outside);
  if (std::string(is_outside) == "1") {
    return longfist::enums::AccountingMethodType::Outside;
  }
  return longfist::enums::AccountingMethodType::Default;
}

class AccountingMethod {
public:
  AccountingMethod() = default;

  virtual ~AccountingMethod() = default;

  virtual void apply_trading_day(Book_ptr &book, int64_t trading_day) = 0;

  virtual void apply_quote(Book_ptr &book, const longfist::types::Quote &quote) = 0;

  virtual void apply_order_input(Book_ptr &book, const longfist::types::OrderInput &input) = 0;

  virtual void apply_order(Book_ptr &book, const longfist::types::Order &order) = 0;

  virtual void apply_trade(Book_ptr &book, const longfist::types::Trade &trade) = 0;

  virtual void update_position(Book_ptr &book, longfist::types::Position &position) = 0;

  static void setup_defaults(Bookkeeper &bookkeeper,
                             const longfist::enums::AccountingMethodType accounting_method_type);
};

DECLARE_PTR(AccountingMethod)
} // namespace kungfu::wingchun::book
#endif // WINGCHUN_ACCOUNTING_H
