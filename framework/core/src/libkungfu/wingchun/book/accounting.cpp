// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/4/4.
//

#include "accounting/bond.hpp"
#include "accounting/crypto.hpp"
#include "accounting/future.hpp"
#include "accounting/repo.hpp"
#include "accounting/stock.hpp"
#include "outsideaccounting/outside_future.hpp"
#include "outsideaccounting/outside_stock.hpp"

using namespace kungfu::wingchun;

namespace kungfu::wingchun::book {
void AccountingMethod::setup_defaults(Bookkeeper &bookkeeper,
                                      const longfist::enums::AccountingMethodType accounting_method_type) {
  auto bond_accounting_method = std::make_shared<BondAccountingMethod>();
  auto repo_accounting_method = std::make_shared<RepoAccountingMethod>();
  auto crypto_accounting_method = std::make_shared<CryptoAccountingMethod>();

  bookkeeper.set_accounting_method(InstrumentType::Bond, bond_accounting_method);
  bookkeeper.set_accounting_method(InstrumentType::Repo, repo_accounting_method);
  bookkeeper.set_accounting_method(InstrumentType::Crypto, crypto_accounting_method);
  bookkeeper.set_accounting_method(InstrumentType::CryptoFuture, crypto_accounting_method);
  bookkeeper.set_accounting_method(InstrumentType::CryptoUFuture, crypto_accounting_method);

  if (accounting_method_type == longfist::enums::AccountingMethodType::Outside) {
    auto outside_stock_accounting_method = std::make_shared<OutsideStockAccountingMethod>();
    auto outside_future_accounting_method = std::make_shared<OutsideFutureAccountingMethod>();

    bookkeeper.set_accounting_method(InstrumentType::Stock, outside_stock_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::Fund, outside_stock_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::TechStock, outside_stock_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::Index, outside_stock_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::Unknown, outside_stock_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::StockOption, outside_future_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::Future, outside_future_accounting_method);
  } else {
    auto stock_accounting_method = std::make_shared<StockAccountingMethod>();
    auto future_accounting_method = std::make_shared<FutureAccountingMethod>();

    bookkeeper.set_accounting_method(InstrumentType::Stock, stock_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::Fund, stock_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::TechStock, stock_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::Index, stock_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::Unknown, stock_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::StockOption, future_accounting_method);
    bookkeeper.set_accounting_method(InstrumentType::Future, future_accounting_method);
  }
}
} // namespace kungfu::wingchun::book