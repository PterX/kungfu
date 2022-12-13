
// SPDX-License-Identifier: Apache-2.0

//
// Created by Wei Jianan on 2022-12-10
//

#ifndef WINGCHUN_OPERATOR_CONTEXT_H
#define WINGCHUN_OPERATOR_CONTEXT_H
#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/broker/client.h>
// #include <kungfu/wingchun/operator/operator.h>
#include <kungfu/yijinjing/practice/apprentice.h>

namespace kungfu::wingchun::op {
class Context : public std::enable_shared_from_this<Context> {
public:
  Context() = default;

  virtual ~Context() = default;

  /**
   * Get current time in nano seconds.
   * @return current time in nano seconds
   */
  virtual int64_t now() const = 0;

  // /**
  //  * Add one shot timer callback.
  //  * @param nanotime when to call in nano seconds
  //  * @param callback callback function
  //  */
  // virtual void add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) = 0;

  // /**
  //  * Add periodically callback.
  //  * @param duration duration in nano seconds
  //  * @param callback callback function
  //  */
  // virtual void add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) = 0;


  /**
   * Subscribe market data.
   * @param source MD group
   * @param instrument_ids instrument IDs
   * @param exchange_ids exchange IDs
   */
  virtual void subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                         const std::string &exchange_ids) = 0;

  /**
   * Subscribe all from given MD
   * @param source MD group
   */
  virtual void subscribe_all(const std::string &source, uint8_t market_type = 0, uint64_t instrument_type = 0,
                             uint64_t data_type = 0) = 0;

 // TODO added for operator
  /**
   * Subscribe operator data.
   * @param source MD group
   * @param key instrument IDs
   */
  virtual void subscribe(const std::string &source, const std::vector<std::string> &key) = 0;

  /**
   * Get current trading day.
   * @return current trading day
   */
  virtual int64_t get_trading_day() const = 0;

  /**
   * request deregister.
   * @return void
   */
  virtual void req_deregister() {}

// TODO update Operator State
//   /**
//    * Update Strategy State
//    * @param state StrategyState
//    * @param infos vector<string>, info_a, info_b, info_c.
//    */
//   virtual void update_strategy_state(longfist::types::StrategyStateUpdate &state_update) {}


};
} // namespace kungfu::wingchun::op

#endif // WINGCHUN_OPERATOR_CONTEXT_H
