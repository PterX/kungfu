// SPDX-License-Identifier: Apache-2.0

#ifndef WINGCHUN_OPERATOR_RUNTIME_H
#define WINGCHUN_OPERATOR_RUNTIME_H

#include <kungfu/wingchun/operator/context.h>

namespace kungfu::wingchun::op {
class RuntimeContext : public Context {
public:
  explicit RuntimeContext(yijinjing::practice::apprentice &app, const rx::connectable_observable<event_ptr> &events);

  /**
   * Get current time in nano seconds.
   * @return current time in nano seconds
   */
  int64_t now() const override;

  /**
   * Get config from database.
   * @return config of current location_uid
   */
  const std::string get_config() const override;

  /**
   * Add one shot timer callback.
   * @param nanotime when to call in nano seconds
   * @param callback callback function
   */
  int32_t add_timer(int64_t nanotime, const std::function<void(event_ptr)> &callback) override;

  /**
   * Add periodically callback.
   * @param duration duration in nano seconds
   * @param callback callback function
   */
  int32_t add_time_interval(int64_t duration, const std::function<void(event_ptr)> &callback) override;

  /**
   * Clear timer
   * @param timer_id id of timer, return by add_timer and add_time_interval
   */
  void clear_timer(int32_t timer_id) override;

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
  void subscribe_operator(const std::string &group, const std::string &name) override;

  /**
   * publish operator data.
   * @param key key of data to be published
   * @param value value of data to be published
   */
  void publish_synthetic_data(const std::string &key, const std::string &value) override;

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

  /**
   * Update Strategy State
   * @param state StrategyState
   * @param infos vector<string>, info_a, info_b, info_c.
   */
  void update_operator_state(longfist::types::OperatorStateUpdate &state_update) override;

  /**
   * Get subscribed MD locations.
   * @return subscribed MD locations
   */
  const yijinjing::data::location_map &list_md() const;

  /**
   * Get subscribed OPERATOR locations.
   * @return subscribed OPERATOR locations
   */
  const yijinjing::data::location_map &list_op() const;

  /**
   * Get broker client.
   * @return broker client reference
   */
  broker::PassiveClient &get_broker_client();

  void check_dependency_state(const event_ptr &event);

protected:
  // those 3 member maybe shared with BacktestContext
  yijinjing::practice::apprentice &app_;
  const rx::connectable_observable<event_ptr> &events_;

  const yijinjing::data::location_ptr &
  find_location(const std::string &source, longfist::enums::category c,
                std::unordered_map<std::string, yijinjing::data::location_ptr> &locations);

  const yijinjing::data::location_ptr &find_md_location(const std::string &source);

  void on_start() override;

private:
  broker::PassiveClient broker_client_;
  yijinjing::data::location_map md_locations_ = {};
  yijinjing::data::location_map op_locations_ = {};
  std::unordered_map<std::string, yijinjing::data::location_ptr> market_data_ = {};
  std::unordered_map<std::string, yijinjing::data::location_ptr> operator_data_ = {};
  longfist::enums::OperatorState state_;
};

DECLARE_PTR(RuntimeContext)
} // namespace kungfu::wingchun::op

#endif //  WINGCHUN_OPERATOR_RUNTIME_H
