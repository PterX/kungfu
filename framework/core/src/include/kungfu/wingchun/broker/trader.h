// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2019-06-20.
//

#ifndef WINGCHUN_TRADER_H
#define WINGCHUN_TRADER_H

#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/broker/broker.h>
// #include <kungfu/wingchun/broker/algoorder.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/log.h>
#include <kungfu/yijinjing/practice/apprentice.h>

namespace kungfu::wingchun::broker {

FORWARD_DECLARE_CLASS_PTR(TraderVendor)
FORWARD_DECLARE_CLASS_PTR(Trader)
FORWARD_DECLARE_CLASS_PTR(TraderWriterHook)
FORWARD_DECLARE_CLASS_PTR(AlgoOrderService)

typedef std::unordered_map<uint64_t, state<longfist::types::Order>> OrderMap;
typedef std::unordered_map<uint64_t, state<longfist::types::OrderAction>> OrderActionMap;
typedef std::unordered_map<uint64_t, state<longfist::types::Trade>> TradeMap;
typedef std::unordered_map<uint64_t, state<longfist::types::OrderTrigger>> OrderTriggerMap;
typedef std::unordered_map<uint64_t, state<longfist::types::OrderTriggerAction>> OrderTriggerActionMap;

typedef std::unordered_map<uint64_t, longfist::types::Order> Orders;
typedef std::unordered_map<uint64_t, Orders> SubOrders;
typedef std::unordered_map<uint64_t, state<longfist::types::AlgoOrder>> AlgoOrderMap;
typedef std::unordered_map<uint64_t, state<longfist::types::AlgoOrderInput>> AlgoOrderInputMap;

inline bool is_all_order_finished(const Orders &orders) {
  for (auto &iter : orders) {
    if (not is_final_status(iter.second.status)) {
      return false;
    }
  }
  return true;
}

class AlgoOrderService {
public:

  explicit AlgoOrderService(TraderVendor &vendor);
  virtual ~AlgoOrderService() = default;

  void on_algo_order_input(const event_ptr &event, const longfist::types::AlgoOrderInput &algo_order_input);

  void update_algo_order(const longfist::types::Order &order);

  void update_algo_order(int64_t gen_time, uint32_t source, uint32_t dest,
                         const longfist::types::AlgoOrder &algo_order);

  void cancel_algo_order(const event_ptr &event, const longfist::types::AlgoOrderAction &algo_order_action);

  const AlgoOrderMap &get_algo_orders() const;

  void clean_algo_orders(bool bypass_recover = false);

  void clean_algo_orders(uint32_t source, const longfist::types::AlgoOrderInput &algo_order_input,
                         bool bypass_recover = false);

private:
  TraderVendor &vendor_;
  AlgoOrderMap local_algo_orders_;
  AlgoOrderInputMap local_algo_order_inputs_;
  AlgoOrderMap algo_orders_;
  SubOrders local_sub_orders_;

  void try_update_sub_orders(const longfist::types::Order &order);

  bool check_if_all_order_finished(int64_t algo_order_id);

  Trader& get_service();
};

class TraderWriterHook : public yijinjing::journal::writer_hook {
public:
  TraderWriterHook(TraderVendor &vendor);

  void on_open_frame(int64_t trigger_time, const yijinjing::journal::frame_ptr &frame) override;

  void on_close_frame(int64_t gen_time, const yijinjing::journal::frame_ptr &frame) override;

private:
  TraderVendor &vendor_;

  BrokerService_ptr get_service();

  AlgoOrderService &get_algo_order_service();
};

class TraderVendor : public BrokerVendor {
public:
  TraderVendor(locator_ptr locator, const std::string &group, const std::string &name, bool low_latency,
               const std::string &arguments = {});

  void set_service(Trader_ptr service);

  void on_trading_day(const event_ptr &event, int64_t daytime) override;

  BrokerService_ptr get_service() override;

  AlgoOrderService &get_algo_order_service();
  
  const AlgoOrderService &get_algo_order_service() const;

protected:
  void react() override;

  void on_react() override;

  void on_start() override;

  void on_write_to(const event_ptr &event) override;

private:
  Trader_ptr service_ = {};
  AlgoOrderService algo_order_service_;
  TraderWriterHook_ptr hook_;
};

class Trader : public BrokerService {
  friend class TraderVendor;

public:
  explicit Trader(BrokerVendor &vendor) : BrokerService(vendor){};

  [[nodiscard]] virtual longfist::enums::AccountType get_account_type() const = 0;

  virtual bool insert_block_message(const event_ptr &event);

  virtual bool insert_order_trigger(const event_ptr &event) { return true; }

  virtual bool insert_order(const event_ptr &event) = 0;

  virtual bool insert_batch_orders(const event_ptr &event) { return true; }

  virtual bool insert_algo_order(const event_ptr &event);

  virtual bool cancel_order_trigger(const event_ptr &event) { return true; }

  virtual bool cancel_order(const event_ptr &event) = 0;

  virtual bool cancel_algo_order(const event_ptr &event) { return true; }

  virtual bool req_position() = 0;

  virtual bool req_account() = 0;

  virtual bool req_history_order(const event_ptr &event) { return true; }

  virtual bool req_history_trade(const event_ptr &event) { return true; }

  virtual bool on_strategy_exit(const event_ptr &event) { return true; }

  virtual bool on_custom_event(const event_ptr &event) { return true; }

  virtual void on_band(const event_ptr &event) {}

  virtual void on_time_key_value(const event_ptr &event) {}

  void on_risk_setting();

  /// 此函数自动发送一个空的AssetMargin数据. 两融柜台需要发送一个存有数据的AssetMargin, 请override此函数取消写入.
  /// 并且在使用writer写入完AssetMargin之后调用enable_asset_margin_sync()函数.
  /// 非两融柜台想要取消日志输出请override此函数.
  virtual bool write_default_asset_margin();

  [[maybe_unused]] [[nodiscard]] const std::string &get_account_id() const;

  [[nodiscard]] yijinjing::journal::writer_ptr get_asset_writer() const;

  [[nodiscard]] yijinjing::journal::writer_ptr get_asset_margin_writer() const;

  [[nodiscard]] yijinjing::journal::writer_ptr get_position_writer() const;

  [[nodiscard]] AlgoOrderService &get_algo_order_service();

  const AlgoOrderService &get_algo_order_service() const;

  [[nodiscard]] const AlgoOrderMap &get_algo_orders() const;

  void enable_asset_sync();

  void enable_asset_margin_sync();

  void enable_positions_sync();

  void clear_order_inputs(uint64_t location_uid);

  std::unordered_map<uint64_t, std::vector<longfist::types::OrderInput>> &get_order_inputs() { return order_inputs_; }

  OrderMap &get_orders() { return orders_; }

  void enable_self_detect();

  [[maybe_unused]] void disable_recover();

  virtual void on_recover(){};

protected:
  OrderMap orders_{};
  OrderActionMap actions_{};
  OrderTriggerMap triggers_{};
  OrderTriggerActionMap trigger_actions_{};
  TradeMap trades_ = {};
  bool self_deal_detect_ = false;
  bool disable_recover_ = false;
  std::unordered_map<uint64_t, kungfu::longfist::types::BlockMessage> block_messages_{};
  /// <strategy_uid, OrderInput>, a batch OrderInputs for a strategy
  std::unordered_map<uint64_t, std::vector<longfist::types::OrderInput>> order_inputs_{};
  /// <strategy_uid, batch_flag>, true mean batch mode for this strategy
  std::unordered_map<uint64_t, bool> batch_status_{};
  std::unordered_map<std::string, std::unordered_set<uint64_t>> map_exchange_instrument_to_order_ids_{};

private:
  bool sync_asset_ = false;
  bool sync_asset_margin_ = false;
  bool sync_position_ = false;

  void on_asset_sync();

  void on_position_sync();

  void on_order_input(const event_ptr &event);

  void on_batch_order_tag(const event_ptr &event);

  void recover();

  void deal_write_frame();

  void deal_read_frame();
};
} // namespace kungfu::wingchun::broker

#endif // WINGCHUN_TRADER_H
