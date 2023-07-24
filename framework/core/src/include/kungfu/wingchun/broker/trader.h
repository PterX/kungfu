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
FORWARD_DECLARE_CLASS_PTR(BaseService)
FORWARD_DECLARE_CLASS_PTR(AlgoOrderService)

typedef std::unordered_map<uint64_t, state<longfist::types::Order>> OrderMap;
typedef std::unordered_map<uint64_t, state<longfist::types::OrderAction>> OrderActionMap;
typedef std::unordered_map<uint64_t, state<longfist::types::Trade>> TradeMap;
typedef std::unordered_map<uint64_t, state<longfist::types::OrderTrigger>> OrderTriggerMap;
typedef std::unordered_map<uint64_t, state<longfist::types::OrderTriggerAction>> OrderTriggerActionMap;

typedef std::unordered_map<uint64_t, longfist::types::Order> Orders;
typedef std::unordered_map<uint64_t, Orders> SubOrders;
typedef std::vector<longfist::types::OrderInput> OrderInputs;
typedef std::unordered_map<uint32_t, OrderInputs> SubOrderInputs;
typedef std::unordered_map<uint64_t, state<longfist::types::AlgoOrder>> AlgoOrderMap;
typedef std::unordered_map<uint64_t, state<longfist::types::AlgoOrderInput>> AlgoOrderInputMap;

typedef std::unordered_map<uint64_t, longfist::types::BlockMessage> BlockMessages;

inline bool is_all_order_finished(const Orders &orders) {
  for (auto &iter : orders) {
    if (not is_final_status(iter.second.status)) {
      return false;
    }
  }
  return true;
}

class BaseService {
public:
  explicit BaseService(TraderVendor &vendor) : vendor_(vendor){};
  virtual ~BaseService() = default;

  virtual void on_recover();

protected:
  TraderVendor &vendor_;
  bool recover_done_ = false;

  virtual void on_active() = 0;
  Trader &get_service();
};

class AlgoOrderService : public BaseService {
public:
  explicit AlgoOrderService(TraderVendor &vendor) : BaseService(vendor) {}
  void on_algo_order_input(const event_ptr &event, const longfist::types::AlgoOrderInput &algo_order_input);

  void on_order(const longfist::types::Order &order);

  void on_algo_order(int64_t gen_time, uint32_t source, uint32_t dest, const longfist::types::AlgoOrder &algo_order);

  void cancel_algo_order(const event_ptr &event, const longfist::types::AlgoOrderAction &algo_order_action);

  void clean_algo_orders(bool bypass_recover = false);

  void clean_algo_orders(uint32_t source, const longfist::types::AlgoOrderInput &algo_order_input,
                         bool bypass_recover = false);

  const AlgoOrderMap &get_algo_orders() const;

  void on_active() override;

private:
  AlgoOrderMap local_algo_orders_;
  AlgoOrderMap waiting_record_local_algo_orders_;
  AlgoOrderInputMap local_algo_order_inputs_;
  AlgoOrderMap algo_orders_;
  SubOrders local_sub_orders_;

  void try_update_sub_orders(const longfist::types::Order &order);

  bool check_if_all_order_finished(uint64_t algo_order_id);

  int64_t get_volume_traded(uint64_t algo_order_id);
};

class OrderService : public BaseService {
public:
  explicit OrderService(TraderVendor &vendor) : BaseService(vendor) {}

  void on_order_input(const event_ptr &event, const longfist::types::OrderInput &order_input);

  void on_order(uint32_t source, uint32_t dest, int64_t gen_time, const longfist::types::Order &order);

  void on_trade(uint32_t source, uint32_t dest, int64_t gen_time, const longfist::types::Trade& trade);

  void on_batch_order_tag(const event_ptr &event);

  void on_block_message(const longfist::types::BlockMessage &block_message);

  void clean_orders(bool bypass_recover = false);

  void clean_orders(uint32_t source, const longfist::types::OrderInput &order_input, bool bypass_recover = false);

  const OrderMap& get_orders() const;

  const TradeMap& get_trades() const;

  void on_active() override;

private:
  OrderMap orders_{};
  TradeMap trades_{};
  BlockMessages block_messages_ = {};
  std::unordered_map<uint32_t, bool> batch_status_{};
  SubOrderInputs batch_order_inputs_{};

  void clear_batch_order_inputs(uint32_t location_uid);
};

class TraderWriterHook : public yijinjing::journal::writer_hook {
public:
  TraderWriterHook(TraderVendor &vendor);

  void on_open_frame(int64_t trigger_time, yijinjing::journal::frame_ptr frame) override;

  void on_close_frame(int64_t gen_time, yijinjing::journal::frame_ptr frame) override;

private:
  TraderVendor &vendor_;

  BrokerService_ptr get_service();

  AlgoOrderService &get_algo_order_service();

  OrderService &get_order_service();
};

class TraderVendor : public BrokerVendor {
  friend class BaseService;

public:
  TraderVendor(locator_ptr locator, const std::string &group, const std::string &name, bool low_latency,
               const std::string &arguments = {});

  void set_service(Trader_ptr service);

  void on_trading_day(const event_ptr &event, int64_t daytime) override;

  BrokerService_ptr get_service() override;

  AlgoOrderService &get_algo_order_service();

  const AlgoOrderService &get_algo_order_service() const;

  OrderService &get_order_service();

  const OrderService &get_order_service() const;

  void on_recover();

protected:
  void react() override;

  void on_react() override;

  void on_start() override;

  void on_write_to(const event_ptr &event) override;

  void on_active() override;

private:
  Trader_ptr service_{};
  AlgoOrderService algo_order_service_;
  OrderService order_service_;
  TraderWriterHook_ptr hook_;
};

class Trader : public BrokerService {
  friend class TraderVendor;

public:
  explicit Trader(BrokerVendor &vendor) : BrokerService(vendor){};

  [[nodiscard]] virtual longfist::enums::AccountType get_account_type() const = 0;

  virtual bool insert_order_trigger(const event_ptr &event) { return true; }

  virtual bool insert_order(const event_ptr &event) = 0;

  virtual bool insert_block_order(const event_ptr &event, const longfist::types::BlockMessage &block_message) {
    return true;
  }

  virtual bool insert_batch_orders(const event_ptr &event, const OrderInputs &order_inputs) { return true; }

  virtual bool insert_algo_order(const event_ptr &event);

  virtual bool cancel_order_trigger(const event_ptr &event) { return true; }

  virtual bool cancel_order(const event_ptr &event) = 0;

  virtual bool cancel_algo_order(const event_ptr &event) { return true; }

  virtual bool req_position() = 0;

  virtual bool req_account() = 0;

  virtual bool req_order_trigger() { return true; }

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

  [[nodiscard]] AlgoOrderService &get_algo_order_service();

  [[nodiscard]] const AlgoOrderService &get_algo_order_service() const;

  [[nodiscard]] OrderService &get_order_service();

  [[nodiscard]] const OrderService &get_order_service() const;

  [[nodiscard]] const OrderMap &get_orders() const;
  
  [[nodiscard]] const TradeMap &get_trades() const;

  [[nodiscard]] const AlgoOrderMap &get_algo_orders() const;

  void enable_self_detect();

  [[maybe_unused]] void disable_recover();

  virtual void on_recover(){};

protected:
  OrderTriggerMap triggers_{};
  OrderTriggerActionMap trigger_actions_{};
  bool self_deal_detect_ = false;
  bool disable_recover_ = false;

  /// <strategy_uid, batch_flag>, true mean batch mode for this strategy
  std::unordered_map<std::string, std::unordered_set<uint64_t>> map_exchange_instrument_to_order_ids_{};

private:
  bool sync_asset_ = false;
  bool sync_asset_margin_ = false;
  bool sync_position_ = false;

  void on_asset_sync();

  void on_position_sync();

  void recover();

  void deal_write_frame();

  void deal_read_frame();
};
} // namespace kungfu::wingchun::broker

#endif // WINGCHUN_TRADER_H
