// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/7/20.
//

#ifndef WINGCHUN_BACKTEST_H
#define WINGCHUN_BACKTEST_H

#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/wingchun/strategy/matcher.h>

namespace kungfu::wingchun::strategy {
class BacktestContext : public Context {
public:
  explicit BacktestContext(yijinjing::practice::apprentice &app, const rx::connectable_observable<event_ptr> &events,
                           Matcher_ptr matcher);

  /**
   * checked_ is strated started.
   * @return current time in nano seconds
   */
  virtual bool is_started() const override;

  /**
   * Get current time in nano seconds.
   * @return current time in nano seconds
   */
  int64_t now() const override;

  /**
   * Add one shot timer callback.
   * @param nanotime when to call in nano seconds
   * @param callback callback function
   */
  void add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) override;

  /**
   * Add periodically callback.
   * @param duration duration in nano seconds
   * @param callback callback function
   */
  void add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) override;

  /**
   * Add account for strategy.
   * @param source TD group
   * @param account TD account ID
   */
  void add_account(const std::string &source, const std::string &account) override;

  /**
   * Subscribe market data.
   * @param source MD group
   * @param instrument_ids instrument IDs
   * @param exchange_ids exchange IDs
   */
  void subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                 const std::string &exchange_ids) override;

  /**
   * Subscribe all from given MD
   * @param source MD group
   */
  void subscribe_all(const std::string &source, uint8_t market_type = 0, uint64_t instrument_type = 0,
                     uint64_t data_type = 0) override;

  /**
   * Subscribe operator data.
   * @param group OPERATOR group
   * @param name OPERATOR name
   */
  virtual void subscribe_operator(const std::string &group, const std::string &name) override;

  /**
   * Insert Block Message
   * @param opponent_seat
   * @param match_number
   * @param value
   * @return
   */
  virtual uint64_t insert_block_message(const std::string &source, const std::string &account, uint32_t opponent_seat,
                                        uint64_t match_number, bool is_specific = false) override;

  /**
   * Insert order.
   * @param instrument_id instrument ID
   * @param exchange_id exchange ID
   * @param account account ID
   * @param volume trade volume
   * @param type price type
   * @param side side
   * @param offset Deprecated, defaults to longfist::enums::Offset::Open
   * @param hedge_flag Deprecated, defaults to longfist::enums::HedgeFlag::Speculation
   * @return inserted order ID
   */
  uint64_t insert_order(const std::string &instrument_id, const std::string &exchange_id, const std::string &source,
                        const std::string &account, double limit_price, int64_t volume, longfist::enums::PriceType type,
                        longfist::enums::Side side, longfist::enums::Offset offset,
                        longfist::enums::HedgeFlag hedge_flag = longfist::enums::HedgeFlag::Speculation,
                        bool is_swap = false, uint64_t block_id = 0, uint64_t parent_id = 0) override;

  /**
   * Insert Order
   * @param source
   * @param account
   * @param order_input
   * @return
   */
  uint64_t insert_order_input(const std::string &source, const std::string &account,
                              longfist::types::OrderInput &order_input) override;

  /**
   * Insert Batch Orders
   * @param source
   * @param account
   * @param instrument_ids
   * @param exchange_ids
   * @param limit_prices
   * @param volumes
   * @param types
   * @param sides
   * @param offsets
   * @param hedge_flags
   * @param is_swaps
   * @return
   */
  virtual std::vector<uint64_t>
  insert_batch_orders(const std::string &source, const std::string &account,
                      const std::vector<std::string> &instrument_ids, const std::vector<std::string> &exchange_ids,
                      std::vector<double> limit_prices, std::vector<int64_t> volumes,
                      std::vector<longfist::enums::PriceType> types, std::vector<longfist::enums::Side> sides,
                      std::vector<longfist::enums::Offset> offsets, std::vector<longfist::enums::HedgeFlag> hedge_flags,
                      std::vector<bool> is_swaps) override;

  /**
   * Insert Batch Orders
   * @param source
   * @param account
   * @param order_inputs
   * @return
   */
  virtual std::vector<uint64_t> insert_array_orders(const std::string &source, const std::string &account,
                                                    std::vector<longfist::types::OrderInput> &order_inputs) override;

  /*
   * Insert Basket Orders
   * @param basket_id
   * @param source
   * @param account
   * @param price_type
   * @param price_level
   * @param price_offset
   * @param volume_mode
   * @param total_volume
   */
  virtual uint64_t insert_basket_order(uint64_t basket_id, const std::string &source, const std::string &account,
                                       longfist::enums::Side side, longfist::enums::PriceType price_type,
                                       longfist::enums::PriceLevel price_level, double price_offset = 0,
                                       int64_t volume = 0) override;
  /**
   * Get broker client.
   * @return broker client reference
   */
  virtual broker::Client &get_broker_client() override;

  /**
   * Get bookkeeper.
   * @return bookkeeper reference
   */
  virtual book::Bookkeeper &get_bookkeeper() override;

  /**
   * query history order
   */
  virtual void req_history_order(const std::string &source, const std::string &account,
                                 uint32_t query_num = 0) override;

  /**
   * query history trade
   */
  virtual void req_history_trade(const std::string &source, const std::string &account,
                                 uint32_t query_num = 0) override;

  /**
   * Cancel order.
   * @param order_id order ID
   * @return order action ID
   */
  uint64_t cancel_order(uint64_t order_id) override;

  /**
   * request deregister.
   * @return void
   */
  void req_deregister() override;

  /**
   * Update Strategy State
   * @param state StrategyState
   * @param infos vector<string>, info_a, info_b, info_c.
   */
  void update_strategy_state(longfist::types::StrategyStateUpdate &state_update) override;

  /**
   *
   * @param source td source id
   * @param account td account id
   * @return writer to related td
   */
  virtual yijinjing::journal::writer_ptr get_writer(const std::string &source, const std::string &account) override;

protected:
  virtual void on_start() override;

  virtual void prepare(const event_ptr &event) override;

  yijinjing::data::location_ptr find_md_location(const std::string &source);

  yijinjing::data::location_ptr find_op_location(const std::string &group, const std::string &name);

private:
  broker::PassiveClient broker_client_;
  book::Bookkeeper bookkeeper_;
  Matcher_ptr matcher_;
  std::multimap<int64_t, std::function<void(event_ptr)>> pre_timer_callbacks_ = {};
  std::multimap<int64_t, std::function<void(event_ptr)>> timer_callbacks_ = {};

  void on_timer_check();
};

DECLARE_PTR(BacktestContext)
} // namespace kungfu::wingchun::strategy

#endif // WINGCHUN_BACKTEST_H
