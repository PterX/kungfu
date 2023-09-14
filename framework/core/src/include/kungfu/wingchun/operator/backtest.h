// SPDX-License-Identifier: Apache-2.0

#ifndef WINGCHUN_OPERATOR_BACKTEST_H
#define WINGCHUN_OPERATOR_BACKTEST_H

#include <kungfu/wingchun/operator/context.h>
#include <kungfu/wingchun/tool/report.h>
#include <kungfu/wingchun/tool/sliceindexer.h>
#include <kungfu/wingchun/tool/slicetool.h>

namespace kungfu::wingchun::op {
class BacktestContext : public Context {
public:
  explicit BacktestContext(yijinjing::practice::apprentice &app, const rx::connectable_observable<event_ptr> &events,
                           tool::SliceIndexer_ptr from_indexer, tool::SliceIndexer_ptr to_indexer,
                           tool::Report_ptr report);

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
   * @param exchange_id exchange ID
   */
  void subscribe(const std::string &source, const std::vector<std::string> &instrument_ids,
                 const std::string &exchange_id) override;

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
   * Get broker client.
   * @return broker client reference
   */
  broker::Client &get_broker_client() override;

  yijinjing::data::location_ptr get_location(uint32_t location_uid) override;

protected:
  virtual void on_start() override;

  void prepare(const event_ptr &event) override{};

private:
  struct TimerTask {
    int32_t timer_id;
    std::function<void(event_ptr)> call_back;
    TimerTask(int32_t id, std::function<void(event_ptr)> cb) : timer_id(id), call_back(std::move(cb)){};
  };
  broker::PassiveClient broker_client_;
  tool::SliceIndexer_ptr from_indexer_;
  tool::SliceTool_ptr slice_tool_;
  tool::Report_ptr report_;
  int32_t timer_usage_count_{0};
  std::multimap<int64_t, TimerTask> pre_timer_callbacks_{};
  std::multimap<int64_t, TimerTask> timer_callbacks_{};
  std::map<int64_t, std::vector<yijinjing::data::location_ptr>> lease_locations_{};

  void on_timer_check();
  void lease_expired_check();
  int32_t add_timer_interval_helper(int64_t duration, int32_t timer_id, const std::function<void(event_ptr)> &callback);
};

DECLARE_PTR(BacktestContext)
} // namespace kungfu::wingchun::op

#endif // WINGCHUN_OPERATOR_BACKTEST_H