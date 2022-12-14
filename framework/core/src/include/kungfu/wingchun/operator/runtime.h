// SPDX-License-Identifier: Apache-2.0

//
// Created by Wei Jianan on 2022/12/10.
//

#ifndef WINGCHUN_OPERATOR_RUNTIME_H
#define WINGCHUN_OPERATOR_RUNTIME_H

#include <kungfu/wingchun/operator/context.h>

namespace kungfu::wingchun::op{
class RuntimeContext : public Context {
public:
  explicit RuntimeContext(yijinjing::practice::apprentice &app, const rx::connectable_observable<event_ptr> &events);

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

   // TODO added for operator
  /**
   * Subscribe operator data.
   * @param source MD group
   * @param key instrument IDs
   */
  virtual void subscribe(const std::string &source, const std::vector<std::string> &keys) override;


  /**
   * Get current trading day.
   * @return current trading day
   */
  int64_t get_trading_day() const override;

  /**
   * request deregister.
   * @return void
   */
  void req_deregister() override;

//   /**
//    * Update Strategy State
//    * @param state StrategyState
//    * @param infos vector<string>, info_a, info_b, info_c.
//    */
//   void update_operator_state(longfist::types::StrategyStateUpdate &state_update) override;

// TODO should md separate from operator?
// TODO should list_md declare as virtual function in Context?
  /**
   * Get subscribed MD locations.
   * @return subscribed MD locations
   */
  const yijinjing::data::location_map &list_md() const;

// TODO should get_broker_lient declare as virtual function in Context?
  /**
   * Get broker client.
   * @return broker client reference
   */
  broker::Client &get_broker_client();

protected:
 // those 3 member maybe shared with BacktestContext
  yijinjing::practice::apprentice &app_;
  const rx::connectable_observable<event_ptr> &events_;

  virtual void on_start();
  // TODO should find_md_location declare as virtual function in Context?
  // TODO should md separate from operator?
  const yijinjing::data::location_ptr &find_md_location(const std::string &source);

private:
  broker::PassiveClient broker_client_;
  yijinjing::data::location_map md_locations_ = {};
  std::unordered_map<std::string, yijinjing::data::location_ptr> market_data_ = {};
// TODO why these friend functions?
  friend void enable(RuntimeContext &context) { context.on_start(); }
};

DECLARE_PTR(RuntimeContext)
} // namespace kungfu::wingchun::op

#endif //  WINGCHUN_OPERATOR_RUNTIME_H
