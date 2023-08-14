#include <kungfu/common.h>
#include <kungfu/wingchun/broker/trader.h>
#include <kungfu/yijinjing/time.h>

using namespace kungfu::rx;
using namespace kungfu::longfist::types;
using namespace kungfu::longfist::enums;
using namespace kungfu::yijinjing::practice;
using namespace kungfu::yijinjing;
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing::journal;

namespace kungfu::wingchun::broker {

// ====================== BaseService start ======================

Trader &BaseService::get_service() { return dynamic_cast<Trader &>(*vendor_.get_service()); }

// ====================== BaseService end ======================

// ====================== TraderWriterHook start ======================

TraderWriterHook::TraderWriterHook(TraderVendor &vendor) : vendor_(vendor) {}

void TraderWriterHook::on_open_frame(int64_t trigger_time, frame_ptr frame) {}

void TraderWriterHook::on_close_frame(int64_t gen_time, frame_ptr frame) {
  switch (frame->msg_type()) {
  case Order::tag: {
    auto &order = guard_update_time<Order>(frame->data<Order>());
    get_algo_order_service().on_order(frame->gen_time(), frame->source(), frame->dest(), order);
    get_order_service().on_order(frame->gen_time(), frame->source(), frame->dest(), order);
    break;
  }
  case Trade::tag: {
    const Trade &trade = frame->data<Trade>();
    get_order_service().on_trade(frame->gen_time(), frame->source(), frame->dest(), trade);
    get_algo_order_service().on_trade(frame->gen_time(), frame->source(), frame->dest(), trade);
    break;
  }
  case OrderTrigger::tag: {
    auto &order_trigger = guard_update_time<OrderTrigger>(frame->data<OrderTrigger>());
    get_order_trigger_service().on_order_trigger(frame->gen_time(), frame->source(), frame->dest(), order_trigger);
    break;
  }
  case AlgoOrder::tag: {
    auto &algo_order = guard_update_time<AlgoOrder>(frame->data<AlgoOrder>());
    get_algo_order_service().on_algo_order(frame->gen_time(), frame->source(), frame->dest(), algo_order);
    break;
  }
  }
}

BrokerService_ptr TraderWriterHook::get_service() { return vendor_.get_service(); }

OrderService &TraderWriterHook::get_order_service() { return vendor_.get_order_service(); }

OrderTriggerService &TraderWriterHook::get_order_trigger_service() { return vendor_.get_order_trigger_service(); }

AlgoOrderService &TraderWriterHook::get_algo_order_service() { return vendor_.get_algo_order_service(); }

// ====================== TraderWriterHook end ======================

// ====================== TraderVendor start ======================

TraderVendor::TraderVendor(locator_ptr locator, const std::string &group, const std::string &name, bool low_latency,
                           const std::string &arguments)
    : BrokerVendor(location::make_shared(mode::LIVE, category::TD, group, name, std::move(locator)), low_latency,
                   arguments),
      algo_order_service_(*this), order_service_(*this), order_trigger_service_(*this),
      hook_(std::make_shared<TraderWriterHook>(*this)) {
}

void TraderVendor::set_service(Trader_ptr service) { service_ = std::move(service); }

void TraderVendor::react() {
  events_ | skip_until(events_ | is(RequestStart::tag)) | is(OrderInput::tag) |
      $$(order_service_.on_order_input(event));
  events_ | skip_until(events_ | is(RequestStart::tag)) | is_custom() | $$(service_->on_custom_event(event));
  apprentice::react();
}

void TraderVendor::on_start() {
  BrokerVendor::on_start();

  events_ | is(OrderAction::tag) | $$(order_service_.on_order_action(event));
  events_ | is(BlockMessage::tag) | $$(order_service_.on_block_message(event->data<BlockMessage>()));
  events_ | is(BatchOrderBegin::tag, BatchOrderEnd::tag) | $$(order_service_.on_batch_order_tag(event));

  events_ | is(OrderTriggerInput::tag) | $$(order_trigger_service_.on_order_trigger_input(event));
  events_ | is(OrderTriggerAction::tag) | $$(order_trigger_service_.on_order_trigger_action(event));

  events_ | is(AlgoOrderInput::tag) | $$(algo_order_service_.on_algo_order_input(event));
  events_ | is(AlgoOrderAction::tag) | $$(algo_order_service_.on_algo_order_action(event));

  events_ | is(AssetRequest::tag) | $$(service_->req_account());
  events_ | is(PositionRequest::tag) | $$(service_->req_position());
  events_ | is(OrderTriggerRequest::tag) | $$(service_->req_order_trigger());
  events_ | is(RequestHistoryOrder::tag) | $$(service_->req_history_order(event));
  events_ | is(RequestHistoryTrade::tag) | $$(service_->req_history_trade(event));
  events_ | is(AssetSync::tag) | $$(service_->on_asset_sync());
  events_ | is(PositionSync::tag) | $$(service_->on_position_sync());
  events_ | is(Band::tag) | $$(service_->on_band(event));
  events_ | is(TimeKeyValue::tag) | $$(service_->on_time_key_value(event));
  events_ | is(ResetBookRequest::tag) |
      $([&](const event_ptr &event) { get_writer(location::PUBLIC)->mark(now(), ResetBookRequest::tag); });
  events_ | is(Deregister::tag) | $$(service_->on_strategy_exit(event));

  add_time_interval(5 * time_unit::NANOSECONDS_PER_SECOND, [&](auto e) { service_->try_req_account(); });

  service_->on_risk_setting();
  service_->recover();
  on_recover();
  service_->on_start();
}

void TraderVendor::on_write_to(const event_ptr &event) {
  auto dest_id = event->data<RequestWriteTo>().dest_id;
  if (writers_.find(dest_id) == writers_.end()) {
    writers_.emplace(dest_id, get_io_device()->open_hookable_writer(dest_id, hook_));
    if (dest_id == get_master_command_uid()) {
      master_cmd_writer_for_thread_ = get_writer(dest_id);
    }
  }
}

BrokerService_ptr TraderVendor::get_service() { return service_; }

AlgoOrderService &TraderVendor::get_algo_order_service() { return algo_order_service_; }

const AlgoOrderService &TraderVendor::get_algo_order_service() const { return algo_order_service_; }

OrderService &TraderVendor::get_order_service() { return order_service_; }

const OrderService &TraderVendor::get_order_service() const { return order_service_; }

OrderTriggerService &TraderVendor::get_order_trigger_service() { return order_trigger_service_; }

const OrderTriggerService &TraderVendor::get_order_trigger_service() const { return order_trigger_service_; }

void TraderVendor::on_active() {
  order_service_.on_active();
  algo_order_service_.on_active();
  order_trigger_service_.on_active();
}

void TraderVendor::on_recover() {
  algo_order_service_.on_recover();
  service_->on_recover();
}

yijinjing::journal::writer_ptr &TraderVendor::get_thread_writer() {
  if (not thread_writer_) {
    uint32_t dest_id = kungfu::yijinjing::util::get_thread_id();
    thread_writer_ = get_io_device()->open_writer(dest_id);

    /// join channel in sub-thread will crash, so tell master to ask myself to join
    /// do not use writer because of multi-thread concurrency issues
    if (not master_cmd_writer_for_thread_) {
      SPDLOG_ERROR("has no writer of master_cmd: {:8x}:{}", get_master_command_uid(),
                   get_location_uname(get_master_command_uid()));
    }
    RequestReadFromOthers &request = master_cmd_writer_for_thread_->open_data<RequestReadFromOthers>();
    request.source_id = get_home_uid();
    request.dest_id = dest_id;
    request.from_time = now();
    SPDLOG_TRACE("RequestReadFromOthers: {}", request.to_string());
    master_cmd_writer_for_thread_->close_data();
  }
  return thread_writer_;
}
// ====================== TraderVendor end ======================

} // namespace kungfu::wingchun::broker