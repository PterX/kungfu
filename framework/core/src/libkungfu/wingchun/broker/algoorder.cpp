#include <kungfu/wingchun/broker/trader.h>

using namespace kungfu::rx;
using namespace kungfu::wingchun;
using namespace kungfu::wingchun::broker;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::util;

namespace kungfu::wingchun::broker {

void AlgoOrderService::on_algo_order_input(const event_ptr &event,
                                           const longfist::types::AlgoOrderInput &algo_order_input) {
  if (algo_order_input.is_local) {
    SPDLOG_INFO("on_algo_order_input algo_order_id {}", algo_order_input.order_id);
    state<AlgoOrderInput> algo_order_input_state(event->source(), event->dest(), event->gen_time(), algo_order_input);
    local_algo_order_inputs_.insert_or_assign(algo_order_input.order_id, algo_order_input_state);
  }
  get_service().insert_algo_order(event);
}

void AlgoOrderService::on_order(const longfist::types::Order &order) {
  if (not recover_done_)
    return;

  SPDLOG_INFO("on_order order_id {}, parent_id {}", order.order_id, order.parent_id);
  SPDLOG_INFO("111");
  if (order.parent_id == UINT64_ZERO) {
    return;
  }

  SPDLOG_INFO("2222");
  if (local_algo_orders_.find(order.parent_id) == local_algo_orders_.end()) {
    return;
  }

  SPDLOG_INFO("333");
  try_update_sub_orders(order);

  auto &target_algo_order_state = local_algo_orders_.at(order.parent_id);
  auto &target_algo_order = target_algo_order_state.data;
  auto dest = target_algo_order_state.dest;

  auto volume_traded = get_volume_traded(order.parent_id);
  target_algo_order.volume_left = target_algo_order.volume - volume_traded;

  auto has_traded = target_algo_order.volume_left != target_algo_order.volume;
  auto algo_order_is_final = is_final_status(target_algo_order.status);

  if (target_algo_order.volume_left <= 0) {
    target_algo_order.status = OrderStatus::Filled;
  } else if (has_traded && target_algo_order.volume_left > 0 && not algo_order_is_final) {
    target_algo_order.status = OrderStatus::PartialFilledActive;
  }

  SPDLOG_INFO("444");
  SPDLOG_INFO("target_algo_order algo_order_id {}, volume {}, volume_left {}", target_algo_order.order_id,
              target_algo_order.volume, target_algo_order.volume_left);
  SPDLOG_INFO("dest {} {}", dest, vendor_.get_location_uname(dest));
  waiting_record_local_algo_orders_.insert_or_assign(target_algo_order.order_id, target_algo_order_state);
  SPDLOG_INFO("5555");
}

void AlgoOrderService::on_algo_order(int64_t gen_time, uint32_t source, uint32_t dest,
                                     const longfist::types::AlgoOrder &algo_order) {

  // this function fullfill all inner write AlgoOrder demand
  state<AlgoOrder> algo_order_state(source, dest, gen_time, algo_order);
  algo_orders_.insert_or_assign(algo_order.order_id, algo_order_state);

  SPDLOG_INFO("on_algo_order algo_order_id {}, volume {}, volume_left {}", algo_order.order_id, algo_order.volume,
              algo_order.volume_left);
  SPDLOG_INFO("xxxx");
  if (local_algo_order_inputs_.find(algo_order.order_id) != local_algo_order_inputs_.end()) {
    SPDLOG_INFO("000000");
    local_algo_orders_.insert_or_assign(algo_order.order_id, algo_order_state);
  }
};

void AlgoOrderService::try_update_sub_orders(const longfist::types::Order &order) {
  if (local_sub_orders_.find(order.parent_id) == local_sub_orders_.end()) {
    Orders orders;
    local_sub_orders_.emplace(order.parent_id, orders);
  }

  auto &orders = local_sub_orders_.at(order.parent_id);
  orders.insert_or_assign(order.order_id, order);
}

bool AlgoOrderService::check_if_all_order_finished(uint64_t algo_order_id) {
  if (local_sub_orders_.find(algo_order_id) == local_sub_orders_.end()) {
    SPDLOG_ERROR("check_if_all_order_finished no {} in local_sub_orders_", algo_order_id);
    return false;
  }
  auto &orders = local_sub_orders_.at(algo_order_id);
  return is_all_order_finished(orders);
}

int64_t AlgoOrderService::get_volume_traded(uint64_t algo_order_id) {
  if (local_sub_orders_.find(algo_order_id) == local_sub_orders_.end()) {
    SPDLOG_ERROR("get_volume_traded no {} in local_sub_orders_", algo_order_id);
    return false;
  }

  if (local_algo_orders_.find(algo_order_id) == local_algo_orders_.end()) {
    SPDLOG_ERROR("get_volume_traded no {} in local_algo_orders_", algo_order_id);
    return false;
  }

  auto traded_volume = 0;
  auto &orders = local_sub_orders_.at(algo_order_id);
  std::for_each(orders.begin(), orders.end(), [&](auto &pair) {
    auto &order = pair.second;
    SPDLOG_INFO("+++++ order {}", order.to_string());
    traded_volume += order.volume - order.volume_left;
  });

  SPDLOG_INFO("=============== traded_volume {}", traded_volume);
  return traded_volume;
}

void AlgoOrderService::cancel_algo_order(const event_ptr &event,
                                         const longfist::types::AlgoOrderAction &algo_order_action) {
  // no algo order action resolution for local algo order;
  if (local_algo_orders_.find(algo_order_action.order_id) == local_algo_orders_.end()) {
    get_service().cancel_algo_order(event);
    return;
  }

  auto &algo_order_state = local_algo_orders_.at(algo_order_action.order_id);
  auto &algo_order = algo_order_state.data;
  auto dest = algo_order_state.dest;
  auto algo_order_is_final = is_final_status(algo_order.status);
  if (algo_order.volume == algo_order.volume_left) {
    algo_order.status = OrderStatus::Cancelled;
  } else if (not algo_order_is_final) {
    algo_order.status = OrderStatus::PartialFilledNotActive;
  }
  vendor_.get_writer(dest)->write(time::now_in_nano(), algo_order);
}

const AlgoOrderMap &AlgoOrderService::get_algo_orders() const { return algo_orders_; }

void AlgoOrderService::clean_algo_orders(bool bypass_recover) {
  std::for_each(algo_orders_.begin(), algo_orders_.end(), [&](auto &pair) {
    AlgoOrder &algo_order = pair.second.data;

    if (not is_final_status(algo_order.status) and
        (bypass_recover or algo_order.external_order_id.to_string().empty())) {
      algo_order.status = OrderStatus::Lost;
      algo_order.update_time = time::now_in_nano();
      if (vendor_.has_writer(pair.second.dest)) {
        vendor_.write_to(vendor_.now(), algo_order, pair.second.dest);
      }
    }
  });
}

void AlgoOrderService::clean_algo_orders(uint32_t source, const AlgoOrderInput &algo_order_input, bool bypass_recover) {
  if (algo_orders_.find(algo_order_input.order_id) != algo_orders_.end()) {
    return;
  }

  if (not vendor_.has_writer(source)) {
    return;
  }

  AlgoOrder &algo_order = vendor_.get_writer(source)->open_data<AlgoOrder>();
  algo_order_from_input(algo_order_input, algo_order);
  algo_order.status = OrderStatus::Lost;
  algo_order.update_time = time::now_in_nano();
  vendor_.get_writer(source)->close_data();
}

void AlgoOrderService::on_active() {
  if (waiting_record_local_algo_orders_.empty()) {
    return;
  }

  SPDLOG_INFO("on_active");

  auto iter = waiting_record_local_algo_orders_.begin();
  while (iter != waiting_record_local_algo_orders_.end()) {
    auto &algo_order_state = iter->second;
    vendor_.write_to(vendor_.now(), algo_order_state.data, algo_order_state.dest);
    SPDLOG_INFO("================== write algo_order algo_order_id {}, volume {}, volume_left {}, dest {}",
                algo_order_state.data.order_id, algo_order_state.data.volume, algo_order_state.data.volume_left,
                vendor_.get_location_uname(algo_order_state.dest));
    iter = waiting_record_local_algo_orders_.erase(iter);
  }
}

} // namespace kungfu::wingchun::broker
