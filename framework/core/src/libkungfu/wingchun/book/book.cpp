// SPDX-License-Identifier: Apache-2.0

//
// Created by qlu on 2019-10-14.
//

#include <kungfu/wingchun/book/book.h>

#include <utility>

using namespace kungfu::rx;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;

namespace kungfu::wingchun::book {
Book::Book(CommissionMap &commissions_ref, const InstrumentMap &instruments_ref, BasketMap &baskets_ref,
           BasketInstrumentMap &basket_instruments_ref, yijinjing::data::location_ptr home_location)
    : commissions(commissions_ref), instruments(instruments_ref), baskets(baskets_ref),
      basket_instruments(basket_instruments_ref), home(home_location) {}

double Book::get_frozen_price(uint64_t order_id) {
  if (orders.find(order_id) != orders.end()) {
    return orders.at(order_id).frozen_price;
  }
  return 0;
}

void Book::add_source_id(uint32_t source_id) { source_ids.insert(source_id); }

void Book::ensure_position_for(const InstrumentKey &instrument_key) {
  auto apply = [&](auto &position) { return; };
  apply_short_position_for(instrument_key, apply);
  apply_long_position_for(instrument_key, apply);
}

bool Book::has_long_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) const {
  auto position_id = hash_instrument(source_id, exchange_id, instrument_id);
  return long_positions.find(position_id) != long_positions.end();
}

bool Book::has_long_position(const std::string &source, const std::string &account, const char *exchange_id,
                             const char *instrument_id) const {
  auto location = location::make_shared(home->mode, category::TD, source, account, home->locator);
  return has_long_position(location->uid, exchange_id, instrument_id);
}

bool Book::has_short_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) const {
  auto position_id = hash_instrument(source_id, exchange_id, instrument_id);
  return short_positions.find(position_id) != short_positions.end();
}

bool Book::has_short_position(const std::string &source, const std::string &account, const char *exchange_id,
                              const char *instrument_id) const {
  auto location = location::make_shared(home->mode, category::TD, source, account, home->locator);
  return has_short_position(location->uid, exchange_id, instrument_id);
}

bool Book::has_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) const {
  return has_long_position(source_id, exchange_id, instrument_id) or
         has_short_position(source_id, exchange_id, instrument_id);
}

Position &Book::get_long_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) {
  return get_position(source_id, Direction::Long, exchange_id, instrument_id);
}

Position &Book::get_long_position(const std::string &source, const std::string &account, const char *exchange_id,
                                  const char *instrument_id) {
  auto location = location::make_shared(home->mode, category::TD, source, account, home->locator);
  return get_long_position(location->uid, exchange_id, instrument_id);
}

Position &Book::get_short_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) {
  return get_position(source_id, Direction::Short, exchange_id, instrument_id);
}

Position &Book::get_short_position(const std::string &source, const std::string &account, const char *exchange_id,
                                   const char *instrument_id) {
  auto location = location::make_shared(home->mode, category::TD, source, account, home->locator);
  return get_short_position(location->uid, exchange_id, instrument_id);
}

Position &Book::get_position(uint32_t source_id, Direction direction, const char *exchange_id,
                             const char *instrument_id) {
  assert(asset.holder_uid != 0);
  PositionMap &positions = direction == Direction::Long ? long_positions : short_positions;
  auto position_id = hash_instrument(source_id, exchange_id, instrument_id);
  auto pair = positions.try_emplace(position_id);
  auto &position = pair.first->second;
  if (pair.second) {
    position.instrument_id = instrument_id;
    position.exchange_id = exchange_id;
    position.instrument_type = get_instrument_type(position.exchange_id, position.instrument_id);
    position.holder_uid = asset.holder_uid;
    position.ledger_category = asset.ledger_category;
    position.direction = direction;
    position.source_id = source_id;
    position.source_op_id = get_source_op_id(asset.holder_uid, source_id);
  }
  add_source_id(source_id);
  return position;
}

bool Book::has_position(uint32_t source_id, longfist::enums::Direction direction, const char *exchange_id,
                        const char *instrument_id) {
  PositionMap &positions = direction == Direction::Long ? long_positions : short_positions;
  auto position_id = hash_instrument(source_id, exchange_id, instrument_id);
  if (positions.find(position_id) == positions.end()) {
    return false;
  }

  return true;
}

void Book::update(int64_t update_time, longfist::enums::AccountingMethodType accounting_method_type) {
  asset.update_time = update_time;

  /* IMPORTANT:
   * remove assign and reassign of asset.margin
   * this function will be called when ledger sync asset and position from TD  every minute
   * margin will recalculate by this function, but margin of asset is not equal to sum of all positions margin,
   * different exchange may have different margin discount
   */

  // asset.margin = 0;
  asset.market_value = 0;
  asset.short_market_value = 0;
  asset.unrealized_pnl = 0;
  asset.dynamic_equity = asset.avail;

  auto update_position = [&](const Position &position) {
    auto is_stock =
        position.instrument_type == InstrumentType::Stock or position.instrument_type == InstrumentType::Bond or
        position.instrument_type == InstrumentType::Fund or position.instrument_type == InstrumentType::StockOption or
        position.instrument_type == InstrumentType::TechStock or position.instrument_type == InstrumentType::Index or
        position.instrument_type == InstrumentType::Repo;
    auto is_future = position.instrument_type == InstrumentType::Future;

    double db_exchage_rate = DEFAULT_INSTRUMENT_EXCHANGE_RATE;
    double db_contract_multiplier = DEFAULT_INSTRUMENT_CONTRACT_MULTIPLIER;
    auto hashed_instrument_factor_key =
        hash_instrument(position.source_id, position.exchange_id, position.instrument_id);
    if (instrument_factors.find(hashed_instrument_factor_key) != instrument_factors.end()) {
      auto &instrument_factor = instrument_factors.at(hashed_instrument_factor_key);
      db_exchage_rate = is_equal(instrument_factor.exchange_rate, 0.0) ? DEFAULT_INSTRUMENT_EXCHANGE_RATE
                                                                       : instrument_factor.exchange_rate;
    }

    auto hashed_instrument_key = hash_instrument(position.exchange_id, position.instrument_id);
    if (instruments.find(hashed_instrument_key) != instruments.end()) {
      const auto &instrument = instruments.at(hashed_instrument_key);
      db_contract_multiplier = (instrument.contract_multiplier > 0) ? instrument.contract_multiplier
                                                                    : DEFAULT_INSTRUMENT_CONTRACT_MULTIPLIER;
    }

    auto position_market_value = position.volume *
                                 (position.last_price > 0 ? position.last_price : position.avg_open_price) *
                                 db_exchage_rate * db_contract_multiplier;

    asset.market_value += position_market_value;
    asset.unrealized_pnl += position.unrealized_pnl * db_exchage_rate;

    if (is_stock) {
      asset.dynamic_equity += position_market_value;
    } else if (is_future) {
      asset.dynamic_equity += position.margin + position.position_pnl * db_exchage_rate;
    }

    if (position.direction == Direction::Short) {
      asset.short_market_value += position_market_value;
    }
  };

  apply_long_positions(update_position);
  apply_short_positions(update_position);
}

void Book::replace(const OrderInput &input) { order_inputs.insert_or_assign(input.order_id, input); }

void Book::replace(const Order &order) { orders.insert_or_assign(order.order_id, order); }

void Book::replace(const Trade &trade) { trades.insert_or_assign(trade.trade_id, trade); }

void Book::replace(const Commission &commission) {
  uint32_t product_key = yijinjing::util::hash_str_32(commission.product_id);
  commissions.insert_or_assign(product_key, commission);
}

void Book::replace(const longfist::types::InstrumentFactor &instrument_factor) {
  auto instrument_factor_id =
      hash_instrument(instrument_factor.source_id, instrument_factor.exchange_id, instrument_factor.instrument_id);
  instrument_factors.insert_or_assign(instrument_factor_id, instrument_factor);
}

} // namespace kungfu::wingchun::book
