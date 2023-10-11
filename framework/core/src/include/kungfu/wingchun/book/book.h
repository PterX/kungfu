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

// key = hash_str_32(product_id)
typedef std::unordered_map<uint32_t, longfist::types::Commission> CommissionMap;

// key = hash_instrument(exchange_id, instrument_id)
typedef std::unordered_map<uint32_t, longfist::types::Instrument> InstrumentMap;

// key = basket_uid
typedef std::unordered_map<uint32_t, longfist::types::Basket> BasketMap;

// key = hash_basket_instrument(basket_uid, exchange_id, instrument_id)
typedef std::unordered_map<uint32_t, longfist::types::BasketInstrument> BasketInstrumentElement;

// key = basket_uid
typedef std::unordered_map<uint32_t, BasketInstrumentElement> BasketInstrumentMap;

// key = hash_instrument(source_id, exchange_id, instrument_id)
typedef std::unordered_map<uint32_t, longfist::types::Position> PositionMap;

// key = order_id
typedef std::unordered_map<uint64_t, longfist::types::OrderInput> OrderInputMap;

// key = order_id
typedef std::unordered_map<uint64_t, longfist::types::Order> OrderMap;

// key = trade_id
typedef std::unordered_map<uint64_t, longfist::types::Trade> TradeMap;

// key = hash_instrument(source_id, exchange_id, instrument_id)
typedef std::unordered_map<uint32_t, longfist::types::InstrumentFactor> InstrumentFactorMap;

struct Book {
  const CommissionMap &commissions;
  const InstrumentMap &instruments;
  const InstrumentFactorMap &instrument_factors;
  const BasketMap &baskets;
  const BasketInstrumentMap &basket_instruments;
  longfist::types::Asset asset = {};
  PositionMap long_positions = {};
  PositionMap short_positions = {};
  std::unordered_set<uint32_t> source_ids = {};
  OrderInputMap order_inputs = {};
  OrderMap orders = {};
  TradeMap trades = {};
  yijinjing::data::location_ptr home;

  Book(const CommissionMap &commissions_ref, const InstrumentMap &instruments_ref,
       const InstrumentFactorMap &instrument_factors_ref, BasketMap &baskets_ref,
       BasketInstrumentMap &basket_instruments_ref, yijinjing::data::location_ptr home_location);

  double get_frozen_price(uint64_t order_id);

  void add_source_id(uint32_t source_id);

  void ensure_position_for(const longfist::types::InstrumentKey &instrument_key);

  template <typename ApplyMethod>
  void apply_position(uint32_t source_id, longfist::enums::Direction direction, const char *exchange_id,
                      const char *instrument_id, ApplyMethod method) {
    auto &position = get_position(source_id, direction, exchange_id, instrument_id);
    method(position);
  }

  template <typename ApplyMethod>
  void apply_position_pure(uint32_t source_id, longfist::enums::Direction direction, const char *exchange_id,
                           const char *instrument_id, ApplyMethod method) {
    PositionMap &positions = direction == longfist::enums::Direction::Long ? long_positions : short_positions;
    auto position_id = hash_instrument(source_id, exchange_id, instrument_id);
    if (positions.find(position_id) != positions.end()) {
      method(positions.at(position_id));
    }
  }

  template <typename ApplyMethod>
  void apply_opposite_position(uint32_t source_id, longfist::enums::Direction direction, const char *exchange_id,
                               const char *instrument_id, ApplyMethod method) {

    auto direction_opposite = direction == longfist::enums::Direction::Long ? longfist::enums::Direction::Short
                                                                            : longfist::enums::Direction::Long;
    auto &position = get_position(source_id, direction_opposite, exchange_id, instrument_id);
    method(position);
  }

  template <typename ApplyMethod>
  void apply_opposite_position_pure(uint32_t source_id, longfist::enums::Direction direction, const char *exchange_id,
                                    const char *instrument_id, ApplyMethod method) {

    PositionMap &positions = direction == longfist::enums::Direction::Long ? short_positions : long_positions;
    auto position_id = hash_instrument(source_id, exchange_id, instrument_id);
    if (positions.find(position_id) != positions.end()) {
      method(positions.at(position_id));
    }
  }

  template <typename TradingData, typename ApplyMethod>
  std::enable_if_t<std::is_same_v<TradingData, longfist::types::Quote>> apply_long_position_for(const TradingData &data,
                                                                                                ApplyMethod method) {
    for (const auto &source_id : source_ids) {
      apply_position_pure(source_id, longfist::enums::Direction::Long, data.exchange_id, data.instrument_id, method);
    }
  }

  template <typename TradingData, typename ApplyMethod>
  std::enable_if_t<not std::is_same_v<TradingData, longfist::types::Quote>>
  apply_long_position_for(const TradingData &data, ApplyMethod method) {
    for (const auto &source_id : source_ids) {
      apply_position(source_id, longfist::enums::Direction::Long, data.exchange_id, data.instrument_id, method);
    }
  }

  template <typename ApplyMethod> void apply_long_positions(ApplyMethod method) {
    for (auto &iter : long_positions) {
      auto &position = iter.second;
      method(position);
    }
  }

  template <typename TradingData, typename ApplyMethod>
  std::enable_if_t<std::is_same_v<TradingData, longfist::types::Quote>>
  apply_short_position_for(const TradingData &data, ApplyMethod method) {
    for (const auto &source_id : source_ids) {
      apply_position_pure(source_id, longfist::enums::Direction::Short, data.exchange_id, data.instrument_id, method);
    }
  }

  template <typename TradingData, typename ApplyMethod>
  std::enable_if_t<not std::is_same_v<TradingData, longfist::types::Quote>>
  apply_short_position_for(const TradingData &data, ApplyMethod method) {
    for (const auto &source_id : source_ids) {
      apply_position(source_id, longfist::enums::Direction::Short, data.exchange_id, data.instrument_id, method);
    }
  }

  template <typename ApplyMethod> void apply_short_positions(ApplyMethod method) {
    for (auto &iter : short_positions) {
      auto &position = iter.second;
      method(position);
    }
  }

  template <typename TradingData, typename ApplyMethod>
  void apply_position_for(longfist::enums::Direction direction, const TradingData &data, ApplyMethod method) {
    for (const auto &source_id : source_ids) {
      apply_position(source_id, direction, data.exchange_id, data.instrument_id, method);
    }
  }

  template <typename TradingData, typename ApplyMethod>
  void apply_position_for(const TradingData &data, ApplyMethod method) {
    auto direction = get_direction(data.instrument_type, data.side, data.offset);
    for (const auto &source_id : source_ids) {
      apply_position(source_id, direction, data.exchange_id, data.instrument_id, method);
    }
  }

  template <typename TradingData, typename ApplyMethod>
  void apply_position_for(uint32_t source_id, const TradingData &data, ApplyMethod method) {
    auto direction = get_direction(data.instrument_type, data.side, data.offset);
    apply_position(source_id, direction, data.exchange_id, data.instrument_id, method);
  }

  template <typename TradingData, typename ApplyMethod>
  void apply_opposite_position_for(longfist::enums::Direction direction, const TradingData &data, ApplyMethod method) {
    for (const auto &source_id : source_ids) {
      apply_opposite_position(source_id, direction, data.exchange_id, data.instrument_id, method);
    }
  }

  template <typename TradingData, typename ApplyMethod>
  void apply_opposite_position_for(const TradingData &data, ApplyMethod method) {
    auto direction = get_direction(data.instrument_type, data.side, data.offset);
    for (const auto &source_id : source_ids) {
      apply_opposite_position(source_id, direction, data.exchange_id, data.instrument_id, method);
    }
  }

  template <typename TradingData, typename ApplyMethod>
  void apply_opposite_position_for(uint32_t source_id, const TradingData &data, ApplyMethod method) {
    auto direction = get_direction(data.instrument_type, data.side, data.offset);
    apply_opposite_position(source_id, direction, data.exchange_id, data.instrument_id, method);
  }

  [[nodiscard]] bool has_long_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) const;

  [[nodiscard]] bool has_long_position(const std::string &source, const std::string &account, const char *exchange_id,
                                       const char *instrument_id) const;

  [[nodiscard]] bool has_short_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) const;

  [[nodiscard]] bool has_short_position(const std::string &source, const std::string &account, const char *exchange_id,
                                        const char *instrument_id) const;

  [[nodiscard]] bool has_position(uint32_t source_id, const char *exchange_id, const char *instrument_id) const;

  [[nodiscard]] longfist::types::Position &get_long_position(uint32_t source_id, const char *exchange_id,
                                                             const char *instrument_id);

  [[nodiscard]] longfist::types::Position &get_long_position(const std::string &source, const std::string &account,
                                                             const char *exchange_id, const char *instrument_id);

  [[nodiscard]] longfist::types::Position &get_short_position(uint32_t source_id, const char *exchange_id,
                                                              const char *instrument_id);

  [[nodiscard]] longfist::types::Position &get_short_position(const std::string &source, const std::string &account,
                                                              const char *exchange_id, const char *instrument_id);

  [[nodiscard]] longfist::types::Position &get_position(uint32_t source_id, longfist::enums::Direction direction,
                                                        const char *exchange_id, const char *instrument_id);

  [[nodiscard]] bool has_position(uint32_t source_id, longfist::enums::Direction direction, const char *exchange_id,
                                  const char *instrument_id);

  template <typename TradingData>
  [[nodiscard]] longfist::types::Position &get_position_for(uint32_t source_id, const TradingData &data) {
    auto direction = get_direction(data.instrument_type, data.side, data.offset);
    return get_position(source_id, direction, data.exchange_id, data.instrument_id);
  }

  template <typename TradingData> [[nodiscard]] bool has_long_position_for(const TradingData &data) {
    auto result = false;
    for (const auto &source_id : source_ids) {
      result |= has_position(source_id, longfist::enums::Direction::Long, data.exchange_id, data.instrument_id);
    }
    return result;
  }

  template <typename TradingData> [[nodiscard]] bool has_short_position_for(const TradingData &data) {
    auto result = false;
    for (const auto &source_id : source_ids) {
      result |= has_position(source_id, longfist::enums::Direction::Short, data.exchange_id, data.instrument_id);
    }
    return result;
  }

  template <typename TradingData>
  [[nodiscard]] longfist::types::Position &get_opposite_position_for(uint32_t source_id, const TradingData &data) {
    auto direction = get_opposite_direction(data.instrument_type, data.side, data.offset);
    return get_position(source_id, direction, data.exchange_id, data.instrument_id);
  }

  void update(int64_t update_time, longfist::enums::AccountingMethodType accounting_method_type);

  void replace(const longfist::types::OrderInput &input);

  void replace(const longfist::types::Order &order);

  void replace(const longfist::types::Trade &trade);

  [[nodiscard]] const InstrumentMap &get_instruments() const { return instruments; }

  [[nodiscard]] const InstrumentFactorMap &get_instrument_factors() const { return instrument_factors; }

  [[nodiscard]] const CommissionMap &get_commissions() const { return commissions; }

  [[nodiscard]] const BasketMap &get_baskets() const { return baskets; }

  [[nodiscard]] const BasketInstrumentMap &get_basket_instruments() const { return basket_instruments; }

  Book &operator=(const Book &book) { return *this; }
};
} // namespace kungfu::wingchun::book

#endif // WINGCHUN_BOOK_H
