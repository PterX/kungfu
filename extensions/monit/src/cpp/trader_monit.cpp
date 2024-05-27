#include "trader_monit.h"
#include "IMSTradeAPI/pack_types.h"

namespace kungfu::wingchun::monit {

monit::monit(broker::BrokerVendor &vendor) : Trader(vendor) { KUNGFU_SETUP_LOG(); };

monit::~monit(){};

void monit::pre_start() {
  // config_ = nlohmann::json::parse(get_config());
  nlohmann::json config = nlohmann::json::parse(get_config());
  address_ = config["remote_address"];
  group_ = config["remote_group"];
  name_ = config["remote_name"];

  SPDLOG_INFO("config: {}", get_config());
  disable_recover();
}

void monit::on_start() {
  SPDLOG_INFO("start");

  api_ = CICC::API::IMSTradeAPI::CreateApi();
  api_->Initial(address_.c_str());
  api_->SetTradeSPI(this);
  api_->TryLogin(group_.c_str(), name_.c_str(),0);

  update_broker_state(BrokerState::Ready);
};
void monit::on_exit() { SPDLOG_INFO("exit"); };
bool monit::insert_order(const event_ptr &event) { return true; };
bool monit::cancel_order(const event_ptr &event) { return true; };
bool monit::req_position() { return true; };
bool monit::req_account() { return true; };

bool monit::on_custom_event(const event_ptr &event) {
  SPDLOG_DEBUG("msg_type: {}", event->msg_type());
  switch (event->msg_type()) {
  case CICC::types::OrderType:
    return custom_OnOrderEvent(event);
  case CICC::types::TradeType:
    return custom_OnTradeEvent(event);
  case CICC::types::OrderInputType:
    return custom_OnOrderInputEvent(event);
  case CICC::types::OrderActionType:
    return custom_OnOrderActionEvent(event);
  default:
    SPDLOG_ERROR("unrecognized msg_type: {}", event->msg_type());
    return false;
  }
};

bool monit::custom_OnOrderEvent(const event_ptr &event) {
  SPDLOG_DEBUG("custom_OnOrderEvent");
  const kungfu::longfist::types::Order &order = event->data<kungfu::longfist::types::Order>();
  kungfu::longfist::types::Order public_order{};
  memcpy(&public_order, &order, sizeof(kungfu::longfist::types::Order));
  auto writer = get_writer(kungfu::yijinjing::data::location::PUBLIC);
  if (!local_order_.contains(order.order_id)) {
    if (remote_local_order_map_.contains(order.order_id)) {
      public_order.order_id = remote_local_order_map_.at(order.order_id);
    } else {
      SPDLOG_ERROR("can't map remote_order_id:{}", order.order_id);
      return false;
    }
  }

  writer->write(now(), public_order);
  return true;
};
bool monit::custom_OnTradeEvent(const event_ptr &event) {
  SPDLOG_DEBUG("custom_OnTradeEvent");
  const kungfu::longfist::types::Trade &trade = event->data<kungfu::longfist::types::Trade>();
  kungfu::longfist::types::Trade public_trade{};
  memcpy(&public_trade, &trade, sizeof(kungfu::longfist::types::Trade));

  auto writer = get_writer(kungfu::yijinjing::data::location::PUBLIC);
  if (!local_order_.contains(trade.order_id)) {
    if (remote_local_order_map_.contains(trade.order_id)) {
      public_trade.order_id = remote_local_order_map_.at(trade.order_id);
    } else {
      SPDLOG_ERROR("can't map remote_order_id:{}", trade.order_id);
      return false;
    }
  }

  writer->write(now(), public_trade);
  return true;
};
bool monit::custom_OnOrderInputEvent(const event_ptr &event) {
  SPDLOG_DEBUG("custom_OnTradeEvent");
  const kungfu::longfist::types::OrderInput &orderinput = event->data<kungfu::longfist::types::OrderInput>();

  kungfu::longfist::types::OrderInput public_orderinput{};
  memcpy(&public_orderinput, &orderinput, sizeof(kungfu::longfist::types::OrderInput));

  auto writer = get_writer(kungfu::yijinjing::data::location::PUBLIC);
  if (!local_order_.contains(orderinput.order_id)) {
    public_orderinput.order_id = writer->current_frame_uid();
    // writer->write(now(), public_orderinput);
    remote_local_order_map_.emplace(orderinput.order_id, public_orderinput.order_id);
  }
  return true;
};
bool monit::custom_OnOrderActionEvent(const event_ptr &event) { return true; };

void monit::OnRspNewOrder(CICC::types::OrderInput *input) {
  SPDLOG_DEBUG("OnRspNewOrder");
  CICC::types::OrderInput &kf_orderinput =
      get_thread_writer()->open_custom_data<CICC::types::OrderInput>(CICC::types::OrderInputType, now());
  memcpy(&kf_orderinput, input, sizeof(CICC::types::OrderInput));
  get_thread_writer()->close_data();
  return;
};
void monit::OnRspCancelOrder(CICC::types::OrderAction *action) { return; };
void monit::OnNotiOrder(CICC::types::Order *order) {
  SPDLOG_DEBUG("OnNotiOrder");
  CICC::types::Order &kf_order =
      get_thread_writer()->open_custom_data<CICC::types::Order>(CICC::types::OrderType, now());
  memcpy(&kf_order, order, sizeof(CICC::types::Order));
  get_thread_writer()->close_data();
  return;
};
void monit::OnNotiKnock(CICC::types::Trade *trade) {
  SPDLOG_DEBUG("OnNotiKnock");

  auto writer = get_thread_writer();
  CICC::types::Trade &kf_trade = writer->open_custom_data<CICC::types::Trade>(CICC::types::TradeType, now());
  memcpy(&kf_trade, trade, sizeof(CICC::types::Trade));
  writer->close_data();
  return;
  // kungfu::longfist::types::Trade &kf_trade =
  // get_thread_writer()->open_custom_data<kungfu::longfist::types::Trade>(now());
  // memcpy(&kf_trade,trade,sizeof(CICC::types::Trade));
  // get_thread_writer()->close_data();
};

} // namespace kungfu::wingchun::monit