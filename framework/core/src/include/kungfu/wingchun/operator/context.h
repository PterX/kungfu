
// SPDX-License-Identifier: Apache-2.0
#ifndef WINGCHUN_OPERATOR_CONTEXT_H
#define WINGCHUN_OPERATOR_CONTEXT_H

#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/broker/client.h>
#include <kungfu/yijinjing/practice/apprentice.h>
#include <kungfu/wingchun/basketorder/basketorderengine.h>

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

  /**
   * Get config from database.
   * @return  config of current location_uid
   */
  virtual const std::string get_config() const = 0;

  /**
   * Add one shot timer callback.
   * @param nanotime when to call in nano seconds
   * @param callback callback function
   */
  virtual int32_t add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) = 0;

  /**
   * Add periodically callback.
   * @param duration duration in nano seconds
   * @param callback callback function
   */
  virtual int32_t add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) = 0;

  /**
   * Clear timer
   * @param timer_id id of timer, return by add_timer and add_time_interval
   */
  virtual void clear_timer(int32_t timer_id) = 0;

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

  /**
   * Subscribe operator data.
   * @param group OPERATOR group
   * @param name OPERATOR name
   */
  virtual void subscribe_operator(const std::string &group, const std::string &name) = 0;

  /**
   * publish operator data.
   * @param key key of data to be published
   * @param value value of data to be published
   */
  virtual void publish_synthetic_data(const std::string &key, const std::string &value) = 0;

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

  /**
   * Update Strategy State
   * @param state StrategyState
   * @param infos vector<string>, info_a, info_b, info_c.
   */
  virtual void update_operator_state(longfist::types::OperatorStateUpdate &state_update) {}

protected:
  virtual void on_start(){};

private:
  friend void enable(Context &context) { context.on_start(); }
};
} // namespace kungfu::wingchun::op

#endif // WINGCHUN_OPERATOR_CONTEXT_H
