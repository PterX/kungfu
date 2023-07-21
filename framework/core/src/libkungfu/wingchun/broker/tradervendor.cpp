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

TraderWriterHook::TraderWriterHook(TraderVendor &vendor) : vendor_(vendor) {}

void TraderWriterHook::on_open_frame(int64_t trigger_time, const frame_ptr& frame) {}

void TraderWriterHook::on_close_frame(int64_t gen_time, const frame_ptr &frame) {
  if (not vendor_.is_service_started()) {
    return;
  }
  switch (frame->msg_type()) {
  case Order::tag: {
    const Order &order = frame->data<Order>();
    get_algo_order_service().on_order(order);
    break;
  }
  case AlgoOrder::tag: {
    const AlgoOrder &algo_order = frame->data<AlgoOrder>();
    get_algo_order_service().on_algo_order(frame->gen_time(), frame->source(), frame->dest(), algo_order);
    break;
  }
  }
}

BrokerService_ptr TraderWriterHook::get_service() { return vendor_.get_service(); }

AlgoOrderService &TraderWriterHook::get_algo_order_service() { return vendor_.get_algo_order_service(); }

TraderVendor::TraderVendor(locator_ptr locator, const std::string &group, const std::string &name, bool low_latency,
                           const std::string &arguments)
    : BrokerVendor(location::make_shared(mode::LIVE, category::TD, group, name, std::move(locator)), low_latency),
      algo_order_service_(*this), hook_(std::make_shared<TraderWriterHook>(*this)) {
  set_arguments(arguments);
}

void TraderVendor::set_service(Trader_ptr service) { service_ = std::move(service); }

void TraderVendor::react() {
  events_ | skip_until(events_ | is(RequestStart::tag)) | is(OrderInput::tag) | $$(service_->on_order_input(event));
  events_ | skip_until(events_ | is(RequestStart::tag)) | is_custom() | $$(service_->on_custom_event(event));
  apprentice::react();
}

void TraderVendor::on_react() {
  events_ | is(ResetBookRequest::tag) |
      $([&](const event_ptr &event) { get_writer(location::PUBLIC)->mark(now(), ResetBookRequest::tag); });
}

void TraderVendor::on_start() {
  BrokerVendor::on_start();

  events_ | is(BlockMessage::tag) | $$(service_->insert_block_message(event));
  events_ | is(OrderTriggerInput::tag) | $$(service_->insert_order_trigger(event));
  events_ | is(AlgoOrderInput::tag) | $$(algo_order_service_.on_algo_order_input(event, event->data<AlgoOrderInput>()));
  events_ | is(OrderAction::tag) | $$(service_->cancel_order(event));
  events_ | is(OrderTriggerAction::tag) | $$(service_->cancel_order_trigger(event));
  events_ | is(AlgoOrderAction::tag) | $$(algo_order_service_.cancel_algo_order(event, event->data<AlgoOrderAction>()));
  events_ | is(AssetRequest::tag) | $$(service_->req_account());
  events_ | is(Deregister::tag) | $$(service_->on_strategy_exit(event));
  events_ | is(PositionRequest::tag) | $$(service_->req_position());
  events_ | is(RequestHistoryOrder::tag) | $$(service_->req_history_order(event));
  events_ | is(RequestHistoryTrade::tag) | $$(service_->req_history_trade(event));
  events_ | is(AssetSync::tag) | $$(service_->on_asset_sync());
  events_ | is(PositionSync::tag) | $$(service_->on_position_sync());
  events_ | is(Band::tag) | $$(service_->on_band(event));
  events_ | is(TimeKeyValue::tag) | $$(service_->on_time_key_value(event));
  events_ | is(BatchOrderBegin::tag, BatchOrderEnd::tag) | $$(service_->on_batch_order_tag(event));

  service_->on_risk_setting();
  service_->recover();
  service_->on_recover();
  service_->on_start();
  service_started_ = true;
}

void TraderVendor::on_write_to(const event_ptr &event) {
  auto dest_id = event->data<RequestWriteTo>().dest_id;
  if (writers_.find(dest_id) == writers_.end()) {
    writers_.emplace(dest_id, get_io_device()->open_hookable_writer(dest_id, hook_));
  }
}

BrokerService_ptr TraderVendor::get_service() { return service_; }

AlgoOrderService &TraderVendor::get_algo_order_service() { return algo_order_service_; };

const AlgoOrderService &TraderVendor::get_algo_order_service() const { return algo_order_service_; };

void TraderVendor::on_trading_day(const event_ptr &event, int64_t daytime) { service_->on_trading_day(event, daytime); }

bool TraderVendor::is_service_started() const { return service_started_; }

void TraderVendor::on_active() { algo_order_service_.on_active(); }

} // namespace kungfu::wingchun::broker