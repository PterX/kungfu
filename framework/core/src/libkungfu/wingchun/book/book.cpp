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
Book::Book(const CommissionMap &commissions_ref, const InstrumentMap &instruments_ref,
           const InstrumentFactorMap &instrument_factors_ref)
    : commissions(commissions_ref), instruments(instruments_ref), instrument_factors(instrument_factors_ref) {}

double Book::get_frozen_price(uint64_t order_id) {
  if (orders.find(order_id) != orders.end()) {
    return orders.at(order_id).frozen_price;
  }
  return 0;
}

void Book::ensure_position(const InstrumentKey &instrument_key) {
  if (is_shortable(instrument_key.instrument_type)) {
    [[maybe_unused]] const auto &short_position = get_position_for(Direction::Short, instrument_key);
  }
  [[maybe_unused]] auto &long_position = get_position_for(Direction::Long, instrument_key);
}

bool Book::has_long_position(const char *exchange_id, const char *instrument_id) const {
  auto position_id = hash_instrument(exchange_id, instrument_id);
  return long_positions.find(position_id) != long_positions.end();
}

bool Book::has_long_position_volume(const char *exchange_id, const char *instrument_id) const {
  auto position_id = hash_instrument(exchange_id, instrument_id);
  if (long_positions.find(position_id) == long_positions.end()) {
    return false;
  }

  auto &position = long_positions.at(position_id);
  return position.volume != 0;
}

bool Book::has_short_position(const char *exchange_id, const char *instrument_id) const {
  auto position_id = hash_instrument(exchange_id, instrument_id);
  return short_positions.find(position_id) != short_positions.end();
}

bool Book::has_short_position_volume(const char *exchange_id, const char *instrument_id) const {
  auto position_id = hash_instrument(exchange_id, instrument_id);
  if (short_positions.find(position_id) == short_positions.end()) {
    return false;
  }

  auto &position = short_positions.at(position_id);
  return position.volume != 0;
}

bool Book::has_position(const char *exchange_id, const char *instrument_id) const {
  return has_long_position(exchange_id, instrument_id) or has_short_position(exchange_id, instrument_id);
}

Position &Book::get_long_position(const char *exchange_id, const char *instrument_id) {
  return get_position(Direction::Long, exchange_id, instrument_id);
}

Position &Book::get_short_position(const char *exchange_id, const char *instrument_id) {
  return get_position(Direction::Short, exchange_id, instrument_id);
}

Position &Book::get_position(Direction direction, const char *exchange_id, const char *instrument_id) {
  assert(asset.holder_uid != 0);
  PositionMap &positions = direction == Direction::Long ? long_positions : short_positions;
  auto position_id = hash_instrument(exchange_id, instrument_id);
  auto pair = positions.try_emplace(position_id);
  auto &position = pair.first->second;
  if (pair.second) {
    position.trading_day = asset.trading_day;
    position.instrument_id = instrument_id;
    position.exchange_id = exchange_id;
    position.instrument_type = get_instrument_type(position.exchange_id, position.instrument_id);
    position.holder_uid = asset.holder_uid;
    position.ledger_category = asset.ledger_category;
    position.direction = direction;
  }
  return position;
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
  asset_margin.short_market_value = 0;
  asset.unrealized_pnl = 0;
  asset.dynamic_equity = asset.avail;

  auto update_position = [&](Position &position) {
    auto is_stock =
        position.instrument_type == InstrumentType::Stock or position.instrument_type == InstrumentType::Bond or
        position.instrument_type == InstrumentType::Fund or position.instrument_type == InstrumentType::StockOption or
        position.instrument_type == InstrumentType::TechStock or position.instrument_type == InstrumentType::Index or
        position.instrument_type == InstrumentType::Repo;
    auto is_future = position.instrument_type == InstrumentType::Future;

    double db_exchage_rate = DEFAULT_INSTRUMENT_EXCHANGE_RATE;
    double db_contract_multiplier = DEFAULT_INSTRUMENT_CONTRACT_MULTIPLIER;
    auto hashed_instrument_key = hash_instrument(position.exchange_id, position.instrument_id);
    if (instrument_factors.find(hashed_instrument_key) != instrument_factors.end()) {
      auto &instrument_factor = instrument_factors.at(hashed_instrument_key);
      db_exchage_rate = is_equal(instrument_factor.exchange_rate, 0.0) ? DEFAULT_INSTRUMENT_EXCHANGE_RATE
                                                                       : instrument_factor.exchange_rate;
    }

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
      asset_margin.short_market_value += position_market_value;
    }
  };

  for (auto &pair : long_positions) {
    update_position(pair.second);
  }
  for (auto &pair : short_positions) {
    update_position(pair.second);
  }
}

void Book::replace(const OrderInput &input) { order_inputs.insert_or_assign(input.order_id, input); }

void Book::replace(const Order &order) { orders.insert_or_assign(order.order_id, order); }

void Book::replace(const Trade &trade) { trades.insert_or_assign(trade.trade_id, trade); }

void Book::mirror_position_from(const Book &book) {
  auto mirror_position = [&](const PositionMap &source_map) {
    for (auto &source_pair : source_map) {
      longfist::copy(get_position_for(source_pair.second.direction, source_pair.second), source_pair.second);
    }
  };

  long_positions.clear();
  short_positions.clear();
  mirror_position(book.long_positions);
  mirror_position(book.short_positions);
}

} // namespace kungfu::wingchun::book