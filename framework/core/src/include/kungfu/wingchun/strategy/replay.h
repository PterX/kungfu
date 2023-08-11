#ifndef WINGCHUN_STRATEGY_REPLAY_H_
#define WINGCHUN_STRATEGY_REPLAY_H_

#include <kungfu/wingchun/strategy/context.h>
#include <kungfu/yijinjing/journal/journal.h>

namespace kungfu::wingchun::strategy {

class ReplayContext : public Context {
public:
  explicit ReplayContext(yijinjing::practice::apprentice &app, const rx::connectable_observable<event_ptr> &events);

  /**
   * checked_ is strated started.
   * @return current time in nano seconds
   */
  virtual bool is_started() const override;

  /**
   * Get location_uid of current process
   * @return location_uid
   */
  uint32_t get_home_uid() const override;

  /**
   * Get location_uid of current process in live mode
   * @return location_uid
   */
  uint32_t get_live_home_uid() const;

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
   * Insert Block Message
   * @param opponent_seat
   * @param match_number
   * @param value
   * @return
   */
  virtual uint64_t insert_block_message(const std::string &source, const std::string &account,
                                        const std::string &opponent_seat, uint64_t match_number,
                                        bool is_specific = false) override;

  /**
   *
   * @param instrument_id instrument ID
   * @param exchange_id exchange ID
   * @param source source ID
   * @param account account ID
   * @param limit_price limit price
   * @param volume trade volume
   * @param type price type
   * @param side side
   * @param offset offset, defaults to longfist::enums::Offset::Open
   * @param hedge_flag hedge_flag, defaults to longfist::enums::HedgeFlag::Speculation
   * @param block_id BlockMessage id
   * @param is_swap boolean
   * @param parent_id parent order id
   * @return order_id
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
   *
   * @param instrument_id
   * @param exchange_id
   * @param source
   * @param account
   * @param limit_price
   * @param volume
   * @param type
   * @param side
   * @param offset
   * @param trigger_type
   * @param time_condition
   * @param action_flag
   * @param order_id
   * @param stop_price
   * @param hedge_flag
   * @param is_swap
   * @return
   */
  uint64_t insert_order_trigger(const std::string &instrument_id, const std::string &exchange_id,
                                const std::string &source, const std::string &account, double limit_price,
                                int64_t volume, longfist::enums::PriceType type, longfist::enums::Side side,
                                longfist::enums::Offset offset, longfist::enums::OrderTriggerType trigger_type,
                                longfist::enums::TimeCondition time_condition, longfist::enums::ParkedType parked_type,
                                double stop_price = 0,
                                longfist::enums::HedgeFlag hedge_flag = longfist::enums::HedgeFlag::Speculation,
                                bool is_swap = false) override;

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

  /**
   * @param instrument_id instrument ID
   * @param exchange_id exchange ID
   * @param source source ID
   * @param account account ID
   * @param begin_time algo begin time
   * @param end_time algo end time
   * @param volume trade volume
   * @param type price type
   * @param side side
   * @param offset offset, defaults to longfist::enums::Offset::Open
   * @param algo_type_id algo type id
   * @param algo_id algo id
   * @param args json string for algo custom arguments
   * @param is_local boolean marking local algo order
   * @return order_id
   */
  uint64_t insert_algo_order(const std::string &instrument_id, const std::string &exchange_id,
                             const std::string &source, const std::string &account, int64_t begin_time,
                             int64_t end_time, int64_t volume, longfist::enums::PriceType type,
                             longfist::enums::Side side, longfist::enums::Offset offset,
                             const std::string &algo_type_id, const std::string &algo_id, const std::string &args,
                             bool is_local = false) override;

  /**
   * Cancel order.
   * @param order_id order ID
   * @return order action ID
   */
  uint64_t
  cancel_order(uint64_t order_id,
               longfist::enums::OrderActionFlag action_flag = longfist::enums::OrderActionFlag::Cancel) override;

  /**
   * Cancel OrderTrigger
   * @param trigger_id
   * @return trigger action id
   */
  uint64_t cancel_order_trigger(uint64_t trigger_id) override;

  /**
   * Cancel Algo Order
   * @param algo_order_id
   * @return algo order action ID
   */
  uint64_t cancel_algo_order(uint64_t algo_order_id) override;

  /**
   * query history order
   */
  void req_history_order(const std::string &source, const std::string &account, uint32_t query_num = 0) override;

  /**
   * query history trade
   */
  void req_history_trade(const std::string &source, const std::string &account, uint32_t query_num = 0) override;

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

  yijinjing::data::location_ptr get_location(uint32_t location_uid) override;

protected:
  virtual void on_start() override;

  virtual void prepare(const event_ptr &event) override;

private:
  bool positions_set_ = false;

  broker::PassiveClient broker_client_;
  book::Bookkeeper bookkeeper_;
  yijinjing::journal::reader_ptr reader_for_write_;

  yijinjing::journal::frame_ptr read_next(uint32_t msg_type);

  void on_timer_check();
};

DECLARE_PTR(ReplayContext)
} // namespace kungfu::wingchun::strategy

#endif