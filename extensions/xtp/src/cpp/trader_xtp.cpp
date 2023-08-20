//
// Created by qlu on 2019/2/11.
//

#include "trader_xtp.h"
#include "buffer_data.h"
#include "serialize_xtp.h"
#include "type_convert.h"
#include <algorithm>

namespace kungfu::wingchun::xtp {
using namespace kungfu::yijinjing::data;
using namespace kungfu::yijinjing;

TraderXTP::TraderXTP(broker::BrokerVendor &vendor) : Trader(vendor) {
  KUNGFU_SETUP_LOG();
  SPDLOG_DEBUG("arguments: {}", get_vendor().get_arguments());
}

TraderXTP::~TraderXTP() {
  if (api_ != nullptr) {
    api_->Release();
  }
}

void TraderXTP::on_start() {
  config_ = nlohmann::json::parse(get_config());
  SPDLOG_INFO("config: {}", get_config());
  if (config_.client_id < 1 or config_.client_id > 99) {
    SPDLOG_ERROR("client_id must between 1 and 99");
  }
  std::string runtime_folder = get_runtime_folder();
  SPDLOG_INFO("Connecting XTP account {} with tcp://{}:{}", config_.account_id, config_.td_ip, config_.td_port);
  api_ = XTP::API::TraderApi::CreateTraderApi(config_.client_id, runtime_folder.c_str());
  api_->RegisterSpi(this);
  api_->SubscribePublicTopic(XTP_TERT_QUICK);
  api_->SetSoftwareVersion("1.1.0");
  api_->SetSoftwareKey(config_.software_key.c_str());
  session_id_ = api_->Login(config_.td_ip.c_str(), config_.td_port, config_.account_id.c_str(),
                            config_.password.c_str(), XTP_PROTOCOL_TCP);
  if (session_id_ > 0) {
    SPDLOG_INFO("Login successfully");
    req_order_trade();
  } else {
    update_broker_state(BrokerState::LoginFailed);
    SPDLOG_ERROR("Login failed [{}]: {}", api_->GetApiLastError()->error_id, api_->GetApiLastError()->error_msg);
  }
}

void TraderXTP::on_exit() {
  if (api_ != nullptr and session_id_ > 0) {
    auto result = api_->Logout(session_id_);
    SPDLOG_INFO("Logout with return code {}", result);
  }
}

bool TraderXTP::insert_order(const event_ptr &event) {
  const OrderInput &input = event->data<OrderInput>();
  SPDLOG_DEBUG("OrderInput: {}", input.to_string());
  XTPOrderInsertInfo xtp_input = {};
  to_xtp(xtp_input, input);

  SPDLOG_DEBUG("XTPOrderInsertInfo: {}", to_string(xtp_input));
  uint64_t order_xtp_id = api_->InsertOrder(&xtp_input, session_id_);
  auto success = order_xtp_id != 0;

  auto nano = yijinjing::time::now_in_nano();
  auto writer = get_writer(event->source());
  Order &order = writer->open_data<Order>(event->gen_time());
  order_from_input(input, order);
  strncpy(order.external_order_id, std::to_string(order_xtp_id).c_str(), EXTERNAL_ID_LEN);
  order.insert_time = nano;
  order.update_time = nano;

  if (success) {
    map_kf_to_xtp_order_id_.emplace(uint64_t(input.order_id), order_xtp_id);
    map_xtp_to_kf_order_id_.emplace(order_xtp_id, uint64_t(input.order_id));
  } else {
    auto error_info = api_->GetApiLastError();
    order.error_id = error_info->error_id;
    strncpy(order.error_msg, error_info->error_msg, ERROR_MSG_LEN);
    order.status = OrderStatus::Error;
  }

  SPDLOG_DEBUG("Order: {}", order.to_string());
  writer->close_data();
  if (not success) {
    SPDLOG_ERROR("fail to insert order {}, error id {}, {}", to_string(xtp_input), (int)order.error_id,
                 order.error_msg);
  }
  return success;
}

bool TraderXTP::cancel_order(const event_ptr &event) {
  const OrderAction &action = event->data<OrderAction>();
  SPDLOG_DEBUG("OrderAction: {}", action.to_string());
  auto order_id_iter = map_kf_to_xtp_order_id_.find(action.order_id);
  if (order_id_iter == map_kf_to_xtp_order_id_.end()) {
    SPDLOG_ERROR("failed to cancel order {}, can't find related xtp order id", action.order_id);
    return false;
  }

  if (not has_order(action.order_id)) {
    SPDLOG_ERROR("no order_id {} in orders_", action.order_id);
    return false;
  }

  auto &order_state = get_order(action.order_id);
  uint64_t order_xtp_id = order_id_iter->second;
  add_action_id(order_xtp_id, action.order_action_id);
  auto xtp_action_id = api_->CancelOrder(order_xtp_id, session_id_);
  auto success = xtp_action_id != 0;

  if (not success) {
    XTPRI *error_info = api_->GetApiLastError();
    SPDLOG_ERROR("failed to cancel order {}, order_xtp_id: {} session_id: {} error_id: {} error_msg: {}",
                 action.order_id, order_xtp_id, session_id_, error_info->error_id, error_info->error_msg);
    OrderActionError &error = get_writer(event->source())->open_data<OrderActionError>(now());
    error.order_id = action.order_id; // 订单ID
    std::string str_external_order_id = std::to_string(order_xtp_id);
    strncpy(error.external_order_id, str_external_order_id.c_str(), str_external_order_id.length());
    error.order_action_id = action.order_action_id;                                 // 订单操作ID,
    error.error_id = xtp_action_id;                                                 // 错误ID
    strncpy(error.error_msg, error_info->error_msg, strlen(error_info->error_msg)); // 错误信息
    error.insert_time = time::now_in_nano();                                        // 写入时间
    SPDLOG_DEBUG("OrderActionError: {}", error.to_string());
    get_writer(event->source())->close_data();
    return false;
  }

  if (not is_final_status(order_state.data.status)) {
    order_state.data.status = OrderStatus::Cancelling;
  }
  SPDLOG_DEBUG("Order: {}", order_state.data.to_string());
  try_write_to(order_state.data, order_state.dest);
  return success;
}

bool TraderXTP::req_position() {
  SPDLOG_INFO("req_position");
  return api_->QueryPosition(nullptr, session_id_, get_request_id()) == 0;
}

bool TraderXTP::req_account() {
  SPDLOG_INFO("req_account");
  return api_->QueryAsset(session_id_, get_request_id()) == 0;
}

void TraderXTP::OnDisconnected(uint64_t session_id, int reason) {
  if (session_id == session_id_) {
    update_broker_state(BrokerState::DisConnected);
    SPDLOG_ERROR("disconnected, reason: {}", reason);
  }
}

void TraderXTP::OnOrderEvent(XTPOrderInfo *order_info, XTPRI *error_info, uint64_t session_id) {
  if (nullptr == order_info) {
    SPDLOG_ERROR("XTPOrderInfo is nullptr");
    return;
  }
  SPDLOG_DEBUG("XTPOrderInfo: {}", to_string(*order_info));
  auto &bf_order_info = get_thread_writer()->open_custom_data<BufferXTPOrderInfo>(kXTPOrderInfoType, now());
  memcpy(&bf_order_info.order_info, order_info, sizeof(XTPOrderInfo));
  bf_order_info.session_id = session_id;
  if (error_info != nullptr) {
    memcpy(&bf_order_info.error_info, error_info, sizeof(XTPRI));
  } else {
    memset(&bf_order_info.error_info, 0, sizeof(XTPRI));
  }
  SPDLOG_DEBUG("BufferXTPOrderInfo: {}", to_string(bf_order_info));
  get_thread_writer()->close_data();
}

bool TraderXTP::custom_OnOrderEvent(const event_ptr &event) {
  const auto *bf_order_info = reinterpret_cast<const BufferXTPOrderInfo *>(event->data_address());
  return custom_OnOrderEvent(bf_order_info->order_info, bf_order_info->error_info, bf_order_info->session_id);
}

bool TraderXTP::custom_OnOrderEvent(const XTPOrderInfo &order_info, const XTPRI &error_info, uint64_t session_id) {
  SPDLOG_DEBUG("XTPOrderInfo: {}", to_string(order_info));
  SPDLOG_DEBUG("session_id: {}, XTPRI: {}", session_id, to_string(error_info));

  auto order_xtp_id_iter = map_xtp_to_kf_order_id_.find(order_info.order_xtp_id);
  if (order_xtp_id_iter == map_xtp_to_kf_order_id_.end()) {
    SPDLOG_WARN("unrecognized order_xtp_id {}@{}", order_info.order_xtp_id, trading_day_);
    return generate_external_order(order_info);
  }

  uint64_t kf_order_id = order_xtp_id_iter->second;
  if (not has_order(kf_order_id)) {
    return generate_external_order(order_info);
  }

  auto &order_state = get_order(kf_order_id);
  from_xtp(order_info, order_state.data);
  order_state.data.update_time = yijinjing::time::now_in_nano();
  if (error_info.error_id != 0) {
    order_state.data.error_id = error_info.error_id;
    strncpy(order_state.data.error_msg, error_info.error_msg, strlen(error_info.error_msg));
  }
  try_write_to(order_state.data, order_state.dest);
  SPDLOG_DEBUG("Order: {}", order_state.data.to_string());
  try_deal_XTPTradeReport(order_info.order_xtp_id);
  return true;
}

bool TraderXTP::generate_external_order(const XTPOrderInfo &order_info) {
  SPDLOG_DEBUG("XTPOrderInfo: {}", to_string(order_info));
  static const std::unordered_set<int> set_cancel_enum = {
      XTP_ORDER_SUBMIT_STATUS_TYPE::XTP_ORDER_SUBMIT_STATUS_CANCEL_SUBMITTED, //
      XTP_ORDER_SUBMIT_STATUS_TYPE::XTP_ORDER_SUBMIT_STATUS_CANCEL_REJECTED,  //
      XTP_ORDER_SUBMIT_STATUS_TYPE::XTP_ORDER_SUBMIT_STATUS_CANCEL_ACCEPTED   //
  };

  if (not config_.sync_external_order) {
    return false;
  }

  if (set_cancel_enum.find(order_info.order_submit_status) != set_cancel_enum.end()) {
    SPDLOG_DEBUG("this XTPOrderInfo is xtp cancel order, do not generate kungfu Order");
    return false;
  }

  auto writer = get_writer(location::PUBLIC);
  auto nano = yijinjing::time::now_in_nano();
  Order &order = writer->open_data<Order>(now());
  order.order_id = writer->current_frame_uid();
  from_xtp(order_info, order);
  order.insert_time = nano;
  order.update_time = nano;
  map_kf_to_xtp_order_id_.emplace(uint64_t(order.order_id), order_info.order_xtp_id);
  map_xtp_to_kf_order_id_.emplace(order_info.order_xtp_id, uint64_t(order.order_id));
  SPDLOG_DEBUG("Order: {}", order.to_string());
  writer->close_data();
  try_deal_XTPTradeReport(order_info.order_xtp_id);
  return true;
}

void TraderXTP::OnTradeEvent(XTPTradeReport *trade_info, uint64_t session_id) {
  if (nullptr == trade_info) {
    SPDLOG_ERROR("XTPTradeReport is nullptr");
    return;
  }
  SPDLOG_DEBUG("XTPTradeReport: {}", to_string(*trade_info));

  auto &bf_trade_info = get_thread_writer()->open_custom_data<BufferXTPTradeReport>(kXTPTradeReportType, now());
  memcpy(&bf_trade_info.trade_info, trade_info, sizeof(XTPTradeReport));
  bf_trade_info.session_id = session_id;
  SPDLOG_DEBUG("BufferXTPOrderInfo: {}", to_string(bf_trade_info));
  get_thread_writer()->close_data();
}

bool TraderXTP::custom_OnTradeEvent(const XTPTradeReport &trade_info, uint64_t session_id) {
  SPDLOG_DEBUG("XTPTradeReport: {}", to_string(trade_info));
  SPDLOG_DEBUG("session_id: {}", session_id);

  auto order_xtp_id_iter = map_xtp_to_kf_order_id_.find(trade_info.order_xtp_id);
  if (order_xtp_id_iter == map_xtp_to_kf_order_id_.end()) {
    SPDLOG_WARN("unrecognized order_xtp_id {}, store in map_xtp_order_id_to_XTPTradeReports_", trade_info.order_xtp_id);
    add_XTPTradeReport(trade_info);
    return false;
  }

  if (has_dealt_trade(trade_info.order_xtp_id, trade_info.exec_id)) {
    SPDLOG_DEBUG("order_xtp_id:{}, exec_id: {}, has dealt", trade_info.order_xtp_id, trade_info.exec_id);
    return false;
  }

  uint64_t kf_order_id = order_xtp_id_iter->second;
  if (not has_order(kf_order_id)) {
    SPDLOG_ERROR("no order_id {} in orders_", kf_order_id);
    return false;
  }

  add_dealt_trade(trade_info.order_xtp_id, trade_info.exec_id);
  auto &order_state = get_order(kf_order_id);

  if (has_writer(order_state.dest)) {
    auto writer = get_writer(order_state.dest);
    Trade &trade = writer->open_data<Trade>(now());
    from_xtp(trade_info, trade);
    trade.trade_id = writer->current_frame_uid();
    trade.order_id = kf_order_id;
    add_traded_volume(trade_info.order_xtp_id, trade.volume);
    SPDLOG_DEBUG("Trade: {}", trade.to_string());
    writer->close_data();
  } else {
    Trade trade{};
    from_xtp(trade_info, trade);
    trade.trade_id = get_writer(location::PUBLIC)->current_frame_uid() xor (time::now_in_nano() & 0x0000FFFF);
    trade.order_id = kf_order_id;
    add_traded_volume(trade_info.order_xtp_id, trade.volume);
    SPDLOG_DEBUG("Trade: {}", trade.to_string());
    try_write_to(trade, order_state.dest);
  }

  if (not is_final_status(order_state.data.status)) {
    order_state.data.volume_left = std::min<int64_t>(
        order_state.data.volume_left, order_state.data.volume - get_traded_volume(trade_info.order_xtp_id));
    if (order_state.data.volume_left > 0) {
      order_state.data.status = OrderStatus::PartialFilledActive;
    }
    order_state.data.update_time = now();
    SPDLOG_DEBUG("Order: {}", order_state.data.to_string());
    try_write_to(order_state.data, order_state.dest);
  }
  return true;
}

bool TraderXTP::custom_OnTradeEvent(const event_ptr &event) {
  const auto *bf_trade_info = reinterpret_cast<const BufferXTPTradeReport *>(event->data_address());
  return custom_OnTradeEvent(bf_trade_info->trade_info, bf_trade_info->session_id);
}

void TraderXTP::OnCancelOrderError(XTPOrderCancelInfo *cancel_info, XTPRI *error_info, uint64_t session_id) {
  if (nullptr == cancel_info) {
    SPDLOG_ERROR("XTPOrderCancelInfo is nullptr");
    return;
  }
  SPDLOG_DEBUG("XTPOrderCancelInfo: {}", to_string(*cancel_info));

  auto &bf_order_cancel_info =
      get_thread_writer()->open_custom_data<BufferXTPOrderCancelInfo>(kCancelOrderErrorType, now());
  memcpy(&bf_order_cancel_info.cancel_info, cancel_info, sizeof(XTPOrderCancelInfo));
  bf_order_cancel_info.session_id = session_id;
  if (error_info != nullptr) {
    memcpy(&bf_order_cancel_info.error_info, error_info, sizeof(XTPRI));
  } else {
    memset(&bf_order_cancel_info.error_info, 0, sizeof(XTPRI));
  }
  SPDLOG_DEBUG("BufferXTPOrderInfo: {}", to_string(bf_order_cancel_info));
  get_thread_writer()->close_data();
}

bool TraderXTP::custom_OnCancelOrderError(const event_ptr &event) {
  const auto &bf_order_cancel_info = event->custom_data<BufferXTPOrderCancelInfo>();
  return custom_OnCancelOrderError(bf_order_cancel_info.cancel_info, bf_order_cancel_info.error_info,
                                   bf_order_cancel_info.session_id);
}

bool TraderXTP::custom_OnCancelOrderError(const XTPOrderCancelInfo &cancel_info, const XTPRI &error_info,
                                          uint64_t session_id) {
  SPDLOG_DEBUG("XTPOrderCancelInfo: {}", to_string(cancel_info));
  SPDLOG_DEBUG("session_id: {}, XTPRI: {}", session_id, to_string(error_info));

  uint64_t action_id = get_action_id(cancel_info.order_xtp_id);
  if (not has_order_action(action_id)) {
    SPDLOG_WARN("has not related OrderAction of {}:{}", cancel_info.order_xtp_id, action_id);
    return false;
  }

  auto action_state = get_order_action(action_id);
  if (not has_writer(action_state.dest)) {
    SPDLOG_DEBUG("action_state dest: {} is not live, do not write data",
                 get_vendor().get_location_uname(action_state.dest));
    return false;
  }

  auto order_id = action_state.data.order_id;
  if (not has_order(order_id)) {
    SPDLOG_WARN("order_id not in orders_ {}", order_id);
    return false;
  }

  auto order_state = get_order(order_id);
  if (has_writer(order_state.dest)) {
    OrderActionError &error = get_writer(order_state.dest)->open_data<OrderActionError>(now());
    error.order_id = order_state.data.order_id; // 订单ID
    std::string str_external_order_id = std::to_string(cancel_info.order_xtp_id);
    strncpy(error.external_order_id, str_external_order_id.c_str(), str_external_order_id.length());
    error.order_action_id = get_action_id(cancel_info.order_xtp_id);              // 订单操作ID,
    error.error_id = error_info.error_id;                                         // 错误ID
    strncpy(error.error_msg, error_info.error_msg, strlen(error_info.error_msg)); // 错误信息
    error.insert_time = time::now_in_nano();                                      // 写入时间
    SPDLOG_DEBUG("OrderActionError: {}", error.to_string());
    get_writer(order_state.dest)->close_data();
  } else {
    OrderActionError error{};
    error.order_id = order_state.data.order_id; // 订单ID
    std::string str_external_order_id = std::to_string(cancel_info.order_xtp_id);
    strncpy(error.external_order_id, str_external_order_id.c_str(), str_external_order_id.length());
    error.order_action_id = get_action_id(cancel_info.order_xtp_id);              // 订单操作ID,
    error.error_id = error_info.error_id;                                         // 错误ID
    strncpy(error.error_msg, error_info.error_msg, strlen(error_info.error_msg)); // 错误信息
    error.insert_time = time::now_in_nano();                                      // 写入时间
    SPDLOG_DEBUG("OrderActionError: {}", error.to_string());
    try_write_to(error, order_state.dest);
  }
  return true;
}

void TraderXTP::OnQueryPosition(XTPQueryStkPositionRsp *position, XTPRI *error_info, int request_id, bool is_last,
                                uint64_t session_id) {
  if (error_info != nullptr && error_info->error_id != 0) {
    SPDLOG_ERROR("error_id:{}, error_msg: {}, request_id: {}, last: {}", error_info->error_id, error_info->error_msg,
                 request_id, is_last);
    return;
  }
  if (nullptr == position) {
    SPDLOG_ERROR("XTPQueryStkPositionRsp is nullptr");
    return;
  }

  SPDLOG_TRACE("XTPQueryStkPositionRsp: {}", to_string(*position));
  auto writer = get_position_writer();
  Position &stock_pos = writer->open_data<Position>(0);
  if (error_info == nullptr || error_info->error_id == 0) {
    from_xtp(*position, stock_pos);
  }
  stock_pos.holder_uid = get_home_uid();
  stock_pos.source_op_id = get_home_uid();
  stock_pos.source_id = get_home_uid();
  stock_pos.instrument_type = get_instrument_type(stock_pos.exchange_id, stock_pos.instrument_id);
  stock_pos.direction = Direction::Long;
  stock_pos.update_time = yijinjing::time::now_in_nano();
  SPDLOG_TRACE("Position: {}", stock_pos.to_string());
  writer->close_data();
  if (is_last) {
    PositionEnd &end = writer->open_data<PositionEnd>(0);
    end.holder_uid = get_home_uid();
    writer->close_data();
    enable_positions_sync();
  }
}

void TraderXTP::OnQueryAsset(XTPQueryAssetRsp *asset, XTPRI *error_info, int request_id, bool is_last,
                             uint64_t session_id) {
  if (error_info != nullptr && error_info->error_id != 0) {
    SPDLOG_ERROR("error_id: {}, error_msg: {}, request_id: {}, last: {}", error_info->error_id, error_info->error_msg,
                 request_id, is_last);
  }
  if (nullptr == asset) {
    SPDLOG_ERROR("XTPQueryAssetRsp is nullptr");
    return;
  }
  if (error_info == nullptr || error_info->error_id == 0 || error_info->error_id == 11000350) {
    SPDLOG_TRACE("OnQueryAsset: {}", to_string(*asset));
    auto writer = get_asset_writer();
    Asset &account = writer->open_data<Asset>(0);
    if (error_info == nullptr || error_info->error_id == 0) {
      from_xtp(*asset, account);
    }
    account.holder_uid = get_home()->uid;
    account.update_time = yijinjing::time::now_in_nano();
    SPDLOG_TRACE("Asset: {}", account.to_string());
    writer->close_data();
    enable_asset_sync();
  }
}

bool TraderXTP::req_history_order(const event_ptr &event) {
  XTPQueryOrderReq query_param{};
  int request_id = get_request_id();
  int ret = api_->QueryOrders(&query_param, session_id_, request_id);
  if (0 != ret) {
    SPDLOG_ERROR("QueryOrders False: {}", ret);
  }
  map_request_location_.emplace(request_id, event->source());
  return 0 == ret;
}

bool TraderXTP::req_history_trade(const event_ptr &event) {
  XTPQueryTraderReq query_param{};
  int request_id = get_request_id();
  int ret = api_->QueryTrades(&query_param, session_id_, request_id);
  if (0 != ret) {
    SPDLOG_ERROR("QueryTrades False ： {}", ret);
  }
  map_request_location_.emplace(request_id, event->source());
  return 0 == ret;
}

void TraderXTP::OnQueryOrder(XTPQueryOrderRsp *order_info, XTPRI *error_info, int request_id, bool is_last,
                             uint64_t session_id) {
  SPDLOG_DEBUG("request_id: {}, is_last: {}, session_id: {}", request_id, is_last, session_id);
  auto &bf_order_info = get_thread_writer()->open_custom_data<BufferXTPOrderInfo>(kQueryXTPOrderInfoType, now());
  if (order_info != nullptr) {
    memcpy(&bf_order_info.order_info, order_info, sizeof(XTPOrderInfo));
  } else {
    memset(&bf_order_info.order_info, 0, sizeof(XTPOrderInfo));
  }
  if (error_info != nullptr) {
    memcpy(&bf_order_info.error_info, error_info, sizeof(XTPRI));
  } else {
    memset(&bf_order_info.error_info, 0, sizeof(XTPRI));
  }
  bf_order_info.session_id = session_id;
  bf_order_info.request_id = request_id;
  bf_order_info.is_last = is_last;
  SPDLOG_DEBUG("BufferXTPOrderInfo: {}", to_string(bf_order_info));
  get_thread_writer()->close_frame(sizeof(BufferXTPOrderInfo));
}

bool TraderXTP::custom_OnQueryOrder(const event_ptr &event) {
  const auto *bf_order_info = reinterpret_cast<const BufferXTPOrderInfo *>(event->data_address());
  return custom_OnQueryOrder(bf_order_info->order_info, bf_order_info->error_info, bf_order_info->request_id,
                             bf_order_info->is_last, bf_order_info->session_id);
}

bool TraderXTP::custom_OnQueryOrder(const XTPOrderInfo &order_info, const XTPRI &error_info, int request_id,
                                    bool is_last, uint64_t session_id) {
  SPDLOG_DEBUG("XTPOrderInfo: {}", to_string(order_info));
  SPDLOG_DEBUG("XTPRI: {}", to_string(error_info));
  SPDLOG_DEBUG("request_id: {}, is_last: {}", request_id, is_last);

  // 查询历史流水收到nullptr, 经过journal走一圈后表现形式为 order_xtp_id == 0
  if (order_info.order_xtp_id == 0 and is_last and
      map_request_location_.find(request_id) != map_request_location_.end()) {
    SPDLOG_WARN("XTPQueryOrderRsp* order_info == nullptr, no data returned!");
    auto writer = get_history_writer(request_id);
    HistoryOrder &history_order = writer->open_data<HistoryOrder>();
    history_order.is_last = true;
    history_order.data_type = HistoryDataType::TotalEnd;
    const std::string msg = "No order today";
    strncpy(history_order.error_msg, msg.c_str(), msg.length());
    writer->close_data();
    SPDLOG_DEBUG("HistoryOrder: {}", history_order.to_string());
    return false;
  }

  if (map_request_location_.find(request_id) == map_request_location_.end()) {
    // TD重连收到推送当做普通下单委托响应处理
    if (is_last) {
      req_order_over_ = true;
      try_ready();
    }
    return order_info.order_xtp_id != 0 and custom_OnOrderEvent(order_info, error_info, request_id);
  }

  auto writer = get_history_writer(request_id);
  HistoryOrder &history_order = writer->open_data<HistoryOrder>();

  if (error_info.error_id != 0) {
    SPDLOG_ERROR("OnQueryOrder False , error_code : {}, error_msg : {}", error_info.error_id, error_info.error_msg);
    history_order.error_id = error_info.error_id;
    strncpy(history_order.error_msg, error_info.error_msg, ERROR_MSG_LEN);
  }

  from_xtp(order_info, history_order);
  history_order.order_id = writer->current_frame_uid();
  history_order.is_last = is_last;
  history_order.insert_time = yijinjing::time::now_in_nano();
  history_order.update_time = history_order.insert_time;
  SPDLOG_DEBUG("HistoryOrder: {}", history_order.to_string());
  writer->close_data();
  return true;
}

yijinjing::journal::writer_ptr TraderXTP::get_history_writer(uint64_t request_id) {
  return get_writer(map_request_location_.try_emplace(request_id).first->second);
}

void TraderXTP::OnQueryTrade(XTPQueryTradeRsp *trade_info, XTPRI *error_info, int request_id, bool is_last,
                             uint64_t session_id) {
  SPDLOG_DEBUG("request_id: {}, is_last: {}, session_id: {}", request_id, is_last, session_id);
  auto &bf_trade_info = get_thread_writer()->open_custom_data<BufferXTPTradeReport>(kQueryXTPTradeReportType, now());
  if (trade_info != nullptr) {
    memcpy(&bf_trade_info.trade_info, trade_info, sizeof(XTPOrderInfo));
  } else {
    memset(&bf_trade_info.trade_info, 0, sizeof(XTPOrderInfo));
  }
  if (error_info != nullptr) {
    memcpy(&bf_trade_info.error_info, error_info, sizeof(XTPRI));
  } else {
    memset(&bf_trade_info.error_info, 0, sizeof(XTPRI));
  }
  bf_trade_info.session_id = session_id;
  bf_trade_info.request_id = request_id;
  bf_trade_info.is_last = is_last;
  SPDLOG_DEBUG("BufferXTPTradeReport: {}", to_string(bf_trade_info));
  get_thread_writer()->close_data();
}

bool TraderXTP::custom_OnQueryTrade(const event_ptr &event) {
  const auto *bf_trade_info = reinterpret_cast<const BufferXTPTradeReport *>(event->data_address());
  return custom_OnQueryTrade(bf_trade_info->trade_info, bf_trade_info->error_info, bf_trade_info->request_id,
                             bf_trade_info->is_last, bf_trade_info->session_id);
}

bool TraderXTP::custom_OnQueryTrade(const XTPTradeReport &trade_info, const XTPRI &error_info, int request_id,
                                    bool is_last, uint64_t session_id) {
  SPDLOG_DEBUG("XTPTradeReport: {}", to_string(trade_info));
  SPDLOG_DEBUG("XTPRI: {}", to_string(error_info));
  SPDLOG_DEBUG("request_id: {}, is_last: {}", request_id, is_last);

  // 查询历史流水收到nullptr, 经过journal走一圈后表现形式为 order_xtp_id == 0
  if (trade_info.order_xtp_id == 0 and is_last and
      map_request_location_.find(request_id) != map_request_location_.end()) {
    SPDLOG_WARN("XTPQueryTradeRsp* trade_info == nullptr, no data returned!");
    auto writer = get_history_writer(request_id);
    HistoryTrade &history_trade = writer->open_data<HistoryTrade>(now());
    history_trade.is_last = true;
    history_trade.data_type = HistoryDataType::TotalEnd;
    const std::string msg = "No trade today";
    strncpy(history_trade.error_msg, msg.c_str(), msg.length());
    writer->close_data();
    return false;
  }

  if (map_request_location_.find(request_id) == map_request_location_.end()) {
    // TD重连收到推送当做普通交易成交回报推送处理
    if (is_last) {
      req_trade_over_ = true;
      try_ready();
    }
    return trade_info.order_xtp_id != 0 and custom_OnTradeEvent(trade_info, session_id);
  }

  auto writer = get_history_writer(request_id);
  HistoryTrade &history_trade = writer->open_data<HistoryTrade>(now());

  if (error_info.error_id != 0) {
    SPDLOG_ERROR("OnQueryTrade False , error_code : {}, error_msg : {}", error_info.error_id, error_info.error_msg);
    history_trade.error_id = error_info.error_id;
    strncpy(history_trade.error_msg, error_info.error_msg, ERROR_MSG_LEN);
  }

  from_xtp(trade_info, history_trade);
  history_trade.trade_id = writer->current_frame_uid();
  history_trade.is_last = is_last;
  history_trade.trade_time = yijinjing::time::now_in_nano();
  history_trade.instrument_type = get_instrument_type(history_trade.exchange_id, history_trade.instrument_id);
  SPDLOG_DEBUG("HistoryTrade: {}", history_trade.to_string());
  writer->close_data();
  return false;
}

void TraderXTP::on_recover() {
  for (auto &pair : get_orders()) {
    const std::string str_external_order_id = pair.second.data.external_order_id.to_string();
    if (not str_external_order_id.empty()) {
      uint64_t order_id = pair.first;
      uint64_t order_xtp_id = std::stoull(str_external_order_id);
      map_xtp_to_kf_order_id_.emplace(order_xtp_id, order_id);
      map_kf_to_xtp_order_id_.emplace(order_id, order_xtp_id);
    }
  }
  for (auto &pair : get_trades()) {
    SPDLOG_DEBUG("trade: {}", pair.second.data.to_string());
    uint64_t order_xtp_id = std::stoull(pair.second.data.external_order_id);
    map_xtp_order_id_to_xtp_trader_ids_.try_emplace(order_xtp_id)
        .first->second.emplace(pair.second.data.external_trade_id.to_string());
  }
}

void TraderXTP::req_order_trade() {
  if (disable_recover_) {
    return try_ready();
  }

  XTPQueryOrderReq query_order_param{};
  int ret = api_->QueryOrders(&query_order_param, session_id_, get_request_id());
  if (0 != ret) {
    SPDLOG_ERROR("QueryOrders False: {}", ret);
  }

  XTPQueryTraderReq query_trade_param{};
  ret = api_->QueryTrades(&query_trade_param, session_id_, get_request_id());
  if (0 != ret) {
    SPDLOG_ERROR("QueryTrades False ： {}", ret);
  }
}

void TraderXTP::try_ready() {
  if (BrokerState::Ready == get_state()) {
    return;
  }

  SPDLOG_DEBUG("req_order_over_: {}, req_trade_over_: {}", req_order_over_, req_trade_over_);
  if (disable_recover_ or (req_order_over_ and req_trade_over_)) {
    update_broker_state(BrokerState::Ready);
  }
}

void TraderXTP::try_deal_XTPTradeReport(uint64_t xtp_order_id) {
  auto &xtp_trades = map_xtp_order_id_to_XTPTradeReports_.try_emplace(xtp_order_id).first->second;
  for (const auto &xtp_trade : xtp_trades) {
    custom_OnTradeEvent(xtp_trade, session_id_);
  }
  xtp_trades.clear();
}

void TraderXTP::add_XTPTradeReport(const XTPTradeReport &trade_info) {
  map_xtp_order_id_to_XTPTradeReports_.try_emplace(trade_info.order_xtp_id).first->second.push_back(trade_info);
}

bool TraderXTP::has_dealt_trade(uint64_t xtp_order_id, const std::string &exec_id) {
  auto &exec_ids = map_xtp_order_id_to_xtp_trader_ids_.try_emplace(xtp_order_id).first->second;
  return exec_ids.find(exec_id) != exec_ids.end();
}

void TraderXTP::add_dealt_trade(uint64_t xtp_order_id, const std::string &exec_id) {
  map_xtp_order_id_to_xtp_trader_ids_.try_emplace(xtp_order_id).first->second.emplace(exec_id);
}

bool TraderXTP::on_custom_event(const event_ptr &event) {
  SPDLOG_DEBUG("msg_type: {}", event->msg_type());
  switch (event->msg_type()) {
  case kXTPOrderInfoType:
    return custom_OnOrderEvent(event);
  case kXTPTradeReportType:
    return custom_OnTradeEvent(event);
  case kQueryXTPOrderInfoType:
    return custom_OnQueryOrder(event);
  case kQueryXTPTradeReportType:
    return custom_OnQueryTrade(event);
  case kCancelOrderErrorType:
    return custom_OnCancelOrderError(event);
  default:
    SPDLOG_ERROR("unrecognized msg_type: {}", event->msg_type());
    return false;
  }
}

void TraderXTP::add_traded_volume(uint64_t order_xtp_id, int64_t trade_volume) {
  map_xtp_order_id_to_traded_volume_.try_emplace(order_xtp_id).first->second += trade_volume;
}

int64_t TraderXTP::get_traded_volume(uint64_t order_xtp_id) {
  return map_xtp_order_id_to_traded_volume_.try_emplace(order_xtp_id).first->second;
}

void TraderXTP::add_action_id(uint64_t xtp_order_id, int64_t action_id) {
  map_xtp_order_id_to_action_ids_.try_emplace(xtp_order_id).first->second.push(action_id);
}

uint64_t TraderXTP::get_action_id(uint64_t xtp_order_id) {
  auto &action_ids = map_xtp_order_id_to_action_ids_.try_emplace(xtp_order_id).first->second;
  uint64_t action_id = action_ids.front();
  action_ids.pop();
  return action_id;
}

} // namespace kungfu::wingchun::xtp
