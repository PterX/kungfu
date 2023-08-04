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
Book::Book(CommissionMap &commissions_ref, const InstrumentMap &instruments_ref)
    : commissions(commissions_ref), instruments(instruments_ref) {}

double Book::get_frozen_price(uint64_t order_id) {
  if (orders.find(order_id) != orders.end()) {
    return orders.at(order_id).frozen_price;
  }
  return 0;
}

void Book::add_source_id(uint32_t source_id) { source_ids.insert(source_id); }

void Book::ensure_position_for(const InstrumentKey &instrument_key) {
  auto apply = [&](auto &position) { assert(position.volume >= 0); };
  apply_short_position_for(instrument_key, apply);
  apply_long_position_for(instrument_key, apply);
}

bool Book::has_long_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) const {
  auto position_id = hash_instrument(source_id, exchange_id, instrument_id);
  return long_positions.find(position_id) != long_positions.end();
}

bool Book::has_short_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) const {
  auto position_id = hash_instrument(source_id, exchange_id, instrument_id);
  return short_positions.find(position_id) != short_positions.end();
}

bool Book::has_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) const {
  return has_long_position(source_id, exchange_id, instrument_id) or
         has_short_position(source_id, exchange_id, instrument_id);
}

Position &Book::get_long_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) {
  return get_position(source_id, Direction::Long, exchange_id, instrument_id);
}

Position &Book::get_short_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) {
  return get_position(source_id, Direction::Short, exchange_id, instrument_id);
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
    position.source_op_id = source_op_id(asset.holder_uid, source_id);
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
  asset.margin = 0;
  asset.market_value = 0;
  asset.unrealized_pnl = 0;
  asset.dynamic_equity = asset.avail;
  double margin = 0;
  double short_market_value = 0;
  auto update_position = [&](const Position &position) {
    auto is_stock =
        position.instrument_type == InstrumentType::Stock or position.instrument_type == InstrumentType::Bond or
        position.instrument_type == InstrumentType::Fund or position.instrument_type == InstrumentType::StockOption or
        position.instrument_type == InstrumentType::TechStock or position.instrument_type == InstrumentType::Index or
        position.instrument_type == InstrumentType::Repo;
    auto is_future = position.instrument_type == InstrumentType::Future;

    double db_exchage_rate = 1.0;
    double db_contract_multiplier = 1.0;
    auto hashed_instrument_key = hash_instrument(position.source_id, position.exchange_id, position.instrument_id);
    if (instrument_factors.find(hashed_instrument_key) != instrument_factors.end()) {
      auto &instrument_factor = instrument_factors.at(hashed_instrument_key);
      db_exchage_rate = is_equal(instrument_factor.exchange_rate, 0.0) ? 1.0 : instrument_factor.exchange_rate;
    }

    if (instruments.find(hashed_instrument_key) != instruments.end()) {
      const auto &instrument = instruments.at(hashed_instrument_key);
      db_contract_multiplier = instrument.contract_multiplier;
    }

    auto position_market_value =
        position.volume * (position.last_price > 0 ? position.last_price : position.avg_open_price) * db_exchage_rate;

    if (accounting_method_type == longfist::enums::AccountingMethodType::OTC && is_future) {
      position_market_value = position.volume *
                              (position.last_price > 0 ? position.last_price : position.avg_open_price) *
                              db_exchage_rate * db_contract_multiplier;
    }
    margin += position.margin;

    if (!(is_stock and position.direction == Direction::Short)) {
      asset.market_value += position_market_value;
      asset.unrealized_pnl += position.unrealized_pnl * db_exchage_rate;
    }
    if (is_stock) {
      if (position.direction == Direction::Long) {
        asset.dynamic_equity += position_market_value;
      } else {
        short_market_value += position_market_value;
      }

    } else if (is_future) {
      asset.dynamic_equity += position.margin + position.position_pnl * db_exchage_rate;
    }
  };

  apply_long_positions(update_position);
  apply_short_positions(update_position);

  asset.margin = margin;
  asset.short_market_value = short_market_value;
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
