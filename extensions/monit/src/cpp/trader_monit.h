#ifndef KUNGFU_MONIT_EXT_TRADER_H
#define KUNGFU_MONIT_EXT_TRADER_H
#include <kungfu/wingchun/broker/trader.h>
#include <unordered_map>
#include <unordered_set>

#include "IMSTradeAPI/IMSTradeAPI.h"

namespace kungfu::wingchun::monit {

// using namespace kungfu::longfist;
// using namespace kungfu::longfist::types;

struct TDConfiguration {
  std::string account_name;
  std::string remote_address;
};

class monit : public broker::Trader, public CICC::API::IMSTradeSPI {
public:
  explicit monit(broker::BrokerVendor &vendor);
  ~monit() override;
  [[nodiscard]] longfist::enums::AccountType get_account_type() const override {
    return longfist::enums::AccountType::Stock;
  };
  void pre_start() override;
  void on_start() override;
  void on_exit() override;
  bool insert_order(const event_ptr &event) override;
  bool cancel_order(const event_ptr &event) override;
  bool req_position() override;
  bool req_account() override;
  bool on_custom_event(const event_ptr &event) override;

private:
  void OnRspNewOrder(CICC::types::OrderInput *input) override;
  void OnRspCancelOrder(CICC::types::OrderAction *action) override;
  void OnNotiOrder(CICC::types::Order *order) override;
  void OnNotiKnock(CICC::types::Trade *trade) override;

  bool custom_OnOrderEvent(const event_ptr &event);
  bool custom_OnTradeEvent(const event_ptr &event);
  bool custom_OnOrderInputEvent(const event_ptr &event);
  bool custom_OnOrderActionEvent(const event_ptr &event);

private:
  // TDConfiguration config_{};
  std::string address_;
  std::string group_;
  std::string name_;
  CICC::API::IMSTradeAPI *api_{};
  std::unordered_map<uint64_t, uint64_t> remote_local_order_map_;
  std::unordered_set<uint64_t> local_order_;
};

} // namespace kungfu::wingchun::monit

#endif // KUNGFU_MONIT_EXT_TRADER_H