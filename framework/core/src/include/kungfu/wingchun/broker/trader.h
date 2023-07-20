// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2019-06-20.
//

#ifndef WINGCHUN_TRADER_H
#define WINGCHUN_TRADER_H

#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/broker/algoorder.h>
#include <kungfu/wingchun/broker/broker.h>
#include <kungfu/yijinjing/io.h>
#include <kungfu/yijinjing/log.h>
#include <kungfu/yijinjing/practice/apprentice.h>

namespace kungfu::wingchun::broker {

FORWARD_DECLARE_CLASS_PTR(TraderVendor)
FORWARD_DECLARE_CLASS_PTR(Trader)
FORWARD_DECLARE_CLASS_PTR(TraderHook);

class TraderHook : public yijinjing::journal::hook {
public:
  TraderHook(TraderVendor &vendor);

  void pre_write(int64_t trigger_time, const yijinjing::journal::frame_ptr &frame) override;

  void post_write(int64_t gen_time, const yijinjing::journal::frame_ptr &frame) override;

private:
  TraderVendor &vendor_;

  BrokerService_ptr get_service();

  const AlgoOrderService_ptr &get_algo_order_service();
};

class TraderVendor : public BrokerVendor {
public:
  TraderVendor(locator_ptr locator, const std::string &group, const std::string &name, bool low_latency,
               const std::string &arguments = {});

  void set_service(Trader_ptr service);

  void on_trading_day(const event_ptr &event, int64_t daytime) override;

  BrokerService_ptr get_service() override;

  const AlgoOrderService_ptr &get_algo_order_service();

protected:
  void react() override;

  void on_react() override;

  void on_start() override;

  void on_write_to(const event_ptr &event) override;

private:
  Trader_ptr service_ = {};
  TraderHook_ptr hook_;
  AlgoOrderService_ptr algo_order_service_;

  void clean_orders();
};

class Trader : public BrokerService {
  friend class TraderVendor;

public:
  typedef std::unordered_map<uint64_t, state<longfist::types::Order>> OrderMap;
  typedef std::unordered_map<uint64_t, state<longfist::types::OrderAction>> OrderActionMap;
  typedef std::unordered_map<uint64_t, state<longfist::types::Trade>> TradeMap;
  typedef std::unordered_map<uint64_t, state<longfist::types::OrderTrigger>> OrderTriggerMap;
  typedef std::unordered_map<uint64_t, state<longfist::types::OrderTriggerAction>> OrderTriggerActionMap;

  explicit Trader(BrokerVendor &vendor) : BrokerService(vendor){};

  [[nodiscard]] virtual longfist::enums::AccountType get_account_type() const = 0;

  virtual bool insert_block_message(const event_ptr &event);

  virtual bool insert_order_trigger(const event_ptr &event) { return true; }

  virtual bool insert_order(const event_ptr &event) = 0;

  virtual bool insert_batch_orders(const event_ptr &event) { return true; }

  virtual longfist::types::AlgoOrder insert_algo_order(const event_ptr &event) {
    auto &algo_order_input = event->data<longfist::types::AlgoOrderInput>();
    longfist::types::AlgoOrder algo_order{};
    algo_order_from_input(algo_order_input, algo_order);
    if (not algo_order_input.is_local) {
      algo_order.status = longfist::enums::OrderStatus::Error;
      std::string error_msg = "Not support Algo Order";
      strcpy(algo_order.error_msg, error_msg.c_str());
    }
    return algo_order;
  }

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

  void handle_asset_sync();

  void handle_position_sync();

  void handle_order_input(const event_ptr &event);

  void handle_batch_order_tag(const event_ptr &event);

  void recover();

  void deal_write_frame();

  void deal_read_frame();
};
} // namespace kungfu::wingchun::broker

#endif // WINGCHUN_TRADER_H
