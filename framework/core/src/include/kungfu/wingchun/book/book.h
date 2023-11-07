// SPDX-License-Identifier: Apache-2.0

//
// Created by qlu on 2019-10-14.
//

#ifndef WINGCHUN_BOOK_H
#define WINGCHUN_BOOK_H

#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/common.h>

namespace kungfu::wingchun::book {
static constexpr int DEFAULT_INSTRUMENT_CONTRACT_MULTIPLIER = 1;
static constexpr double DEFAULT_INSTRUMENT_EXCHANGE_RATE = 1.0;
static constexpr double DEFAULT_FUTURE_LONG_MARGIN_RATIO = 1.0;
static constexpr double DEFAULT_FUTURE_SHORT_MARGIN_RATIO = 1.0;
static constexpr double DEFAULT_STOCK_LONG_MARGIN_RATIO = 1.0;
static constexpr double DEFAULT_STOCK_SHORT_MARGIN_RATIO = 0.6;
static constexpr double DEFAULT_STOCK_CONVERSION_RATE = 0.7;

FORWARD_DECLARE_STRUCT_PTR(Book)
FORWARD_DECLARE_CLASS_PTR(Bookkeeper)

struct Book {
  const map::CommissionMap &commissions;
  const map::InstrumentMap &instruments;
  const map::InstrumentFactorMap &instrument_factors;
  longfist::types::Asset asset = {};
  longfist::types::AssetMargin asset_margin = {};
  map::PositionMap long_positions = {};
  map::PositionMap short_positions = {};
  map::OrderInputMap order_inputs = {};
  map::OrderMap orders = {};
  map::TradeMap trades = {};

  Book(const map::CommissionMap &commissions_ref, const map::InstrumentMap &instruments_ref,
       const map::InstrumentFactorMap &instrument_factors_ref);

  double get_frozen_price(uint64_t order_id);

  void ensure_position(const longfist::types::InstrumentKey &instrument_key);

  [[nodiscard]] bool has_long_position(const char *exchange_id, const char *instrument_id) const;

  [[nodiscard]] bool has_long_position_volume(const char *exchange_id, const char *instrument_id) const;

  [[nodiscard]] bool has_short_position(const char *exchange_id, const char *instrument_id) const;

  [[nodiscard]] bool has_short_position_volume(const char *exchange_id, const char *instrument_id) const;

  [[nodiscard]] bool has_position(const char *exchange_id, const char *instrument_id) const;

  [[nodiscard]] longfist::types::Position &get_long_position(const char *exchange_id, const char *instrument_id);

  [[nodiscard]] longfist::types::Position &get_short_position(const char *exchange_id, const char *instrument_id);

  [[nodiscard]] longfist::types::Position &get_position(longfist::enums::Direction direction, const char *exchange_id,
                                                        const char *instrument_id);

  template <typename TradingData> [[nodiscard]] bool has_position_for(const TradingData &data) const {
    return has_position(data.exchange_id, data.instrument_id);
  }

  template <typename TradingData> [[nodiscard]] bool has_long_position_for(const TradingData &data) const {
    return has_long_position_volume(data.exchange_id, data.instrument_id);
  }

  template <typename TradingData> [[nodiscard]] bool has_short_position_for(const TradingData &data) const {
    return has_short_position_volume(data.exchange_id, data.instrument_id);
  }

  template <typename TradingData>
  [[nodiscard]] longfist::types::Position &get_position_for(longfist::enums::Direction direction,
                                                            const TradingData &data) {
    return get_position(direction, data.exchange_id, data.instrument_id);
  }

  template <typename TradingData>
  [[nodiscard]] longfist::types::Position &get_oppsite_position_for(longfist::enums::Direction direction,
                                                                    const TradingData &data) {
    return direction == longfist::enums::Direction::Long
               ? get_position(longfist::enums::Direction::Short, data.exchange_id, data.instrument_id)
               : get_position(longfist::enums::Direction::Long, data.exchange_id, data.instrument_id);
  }

  template <typename TradingData> [[nodiscard]] longfist::types::Position &get_position_for(const TradingData &data) {
    auto direction = get_direction(data.instrument_type, data.side, data.offset);
    return get_position(direction, data.exchange_id, data.instrument_id);
  }

  template <typename TradingData>
  [[nodiscard]] longfist::types::Position &get_oppsite_position_for(const TradingData &data) {
    auto direction = get_direction(data.instrument_type, data.side, data.offset);
    return direction == longfist::enums::Direction::Long
               ? get_position(longfist::enums::Direction::Short, data.exchange_id, data.instrument_id)
               : get_position(longfist::enums::Direction::Long, data.exchange_id, data.instrument_id);
  }

  void update(int64_t update_time, longfist::enums::AccountingMethodType accounting_method_type);

  void replace(const longfist::types::OrderInput &input);

  void replace(const longfist::types::Order &order);

  void replace(const longfist::types::Trade &trade);

  void mirror_position_from(const Book &book);

  [[nodiscard]] const map::InstrumentMap &get_instruments() const { return instruments; }

  [[nodiscard]] const map::InstrumentFactorMap &get_instrument_factors() const { return instrument_factors; }

  [[nodiscard]] const map::CommissionMap &get_commissions() const { return commissions; }

  Book &operator=(const Book &book) { return *this; }
};
} // namespace kungfu::wingchun::book

#endif // WINGCHUN_BOOK_H
