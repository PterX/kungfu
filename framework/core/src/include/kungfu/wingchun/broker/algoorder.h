#ifndef WINGCHUN_ALGO_ORDER_SERVICE_H
#define WINGCHUN_ALGO_ORDER_SERVICE_H

#include <kungfu/common.h>
#include <kungfu/wingchun/broker/broker.h>
#include <kungfu/wingchun/common.h>

namespace kungfu::wingchun::broker {

FORWARD_DECLARE_CLASS_PTR(AlgoOrderService)

typedef std::unordered_map<uint64_t, kungfu::state<longfist::types::AlgoOrder>> AlgoOrderMap;
typedef std::unordered_map<uint64_t, std::unordered_map<uint64_t, longfist::types::Order>> SubOrderMap;
typedef std::unordered_map<uint64_t, longfist::types::Order> OrderMap;

inline bool is_all_order_finished(const OrderMap &orders) {
  for (auto &iter : orders) {
    if (not is_final_status(iter.second.status)) {
      return false;
    }
  }
  return true;
}

class AlgoOrderService {
public:
  explicit AlgoOrderService(broker::BrokerVendor &vendor);
  virtual ~AlgoOrderService() = default;

  void update_algo_order(const longfist::types::Order &order);

  void update_algo_order(const event_ptr &event, const longfist::types::AlgoOrderInput &algo_order_input);

  void cancel_algo_order(const event_ptr &event, const longfist::types::AlgoOrderAction &algo_order_action);

private:
  broker::BrokerVendor &vendor_;
  AlgoOrderMap local_algo_orders_;
  SubOrderMap local_sub_orders_;

  BrokerService_ptr get_service();

  void try_update_sub_orders(const longfist::types::Order &order);

  bool check_if_all_order_finished(int64_t algo_order_id);
};

} // namespace kungfu::wingchun::broker

#endif // WINGCHUN_ALGO_ORDER_SERVICE_H