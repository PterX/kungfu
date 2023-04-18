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

TraderXTP::TraderXTP(broker::BrokerVendor &vendor) : Trader(vendor), session_id_(0), request_id_(0), trading_day_("") {
  KUNGFU_SETUP_LOG();
}

TraderXTP::~TraderXTP() {
  if (api_ != nullptr) {
    api_->Release();
  }
}

void TraderXTP::on_start() {
  config_ = nlohmann::json::parse(get_config());
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
  if (config_.self_deal_detect) {
    enable_self_detect();
  }
  if (session_id_ > 0) {
    update_broker_state(BrokerState::Ready);
    SPDLOG_INFO("Login successfully");
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

void TraderXTP::on_trading_day(const event_ptr &event, int64_t daytime) {
  trading_day_ = yijinjing::time::strftime(daytime, KUNGFU_TRADING_DAY_FORMAT);
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
  strncpy(order.trading_day, trading_day_.c_str(), DATE_LEN);
  //  order.external_order_id = order_xtp_id;
  strncpy(order.external_order_id, std::to_string(order_xtp_id).c_str(), EXTERNAL_ID_LEN);
  order.insert_time = nano;
  order.update_time = nano;

  if (success) {
    map_kf_to_xtp_order_id_.emplace(input.order_id, order_xtp_id);
    map_xtp_to_kf_order_id_.emplace(order_xtp_id, input.order_id);
  } else {
    auto error_info = api_->GetApiLastError();
    order.error_id = error_info->error_id;
    strncpy(order.error_msg, error_info->error_msg, ERROR_MSG_LEN);
    order.status = OrderStatus::Error;
  }

  orders_.emplace(order.uid(), state<Order>(event->dest(), event->source(), nano, order));
  writer->close_data();
  SPDLOG_DEBUG("Order: {}", order.to_string());
  if (not success) {
    SPDLOG_ERROR("fail to insert order {}, error id {}, {}", to_string(xtp_input), (int)order.error_id,
                 order.error_msg);
  }
  return success;
}

bool TraderXTP::cancel_order(const event_ptr &event) {
  const OrderAction &action = event->data<OrderAction>();
  if (map_kf_to_xtp_order_id_.find(action.order_id) == map_kf_to_xtp_order_id_.end()) {
    SPDLOG_ERROR("failed to cancel order {}, can't find related xtp order id", action.order_id);
    return false;
  }
  uint64_t order_xtp_id = map_kf_to_xtp_order_id_.at(action.order_id);
  const auto &order_state = orders_.at(action.order_id);
  auto xtp_action_id = api_->CancelOrder(order_xtp_id, session_id_);
  auto success = xtp_action_id != 0;
  if (not success) {
    XTPRI *error_info = api_->GetApiLastError();
    SPDLOG_ERROR("failed to cancel order {}, order_xtp_id: {} session_id: {} error_id: {} error_msg: {}",
                 action.order_id, order_xtp_id, session_id_, error_info->error_id, error_info->error_msg);
  }
  return success;
}

bool TraderXTP::req_position() {
  SPDLOG_INFO("req_position");
  return api_->QueryPosition(nullptr, this->session_id_, get_request_id()) == 0;
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

  auto frame = get_thread_writer()->open_frame(now(), kXTPOrderInfoType_, sizeof(buffer_XTPOrderInfo));
  auto *bf_order_info =
      const_cast<buffer_XTPOrderInfo *>(reinterpret_cast<const buffer_XTPOrderInfo *>(frame->data_address()));
  memcpy(&bf_order_info->order_info, order_info, sizeof(XTPOrderInfo));
  bf_order_info->session_id = session_id;
  if (error_info != nullptr) {
    memcpy(&bf_order_info->error_info, error_info, sizeof(XTPRI));
  } else {
    memset(&bf_order_info->error_info, 0, sizeof(XTPRI));
  }
  get_thread_writer()->close_frame(sizeof(buffer_XTPOrderInfo));
  SPDLOG_WARN("buffer_XTPOrderInfo: {}", to_string(*bf_order_info));
}

bool TraderXTP::custom_OnOrderEvent(const XTPOrderInfo &order_info, const XTPRI &error_info, uint64_t session_id) {
  if (map_xtp_to_kf_order_id_.find(order_info.order_xtp_id) == map_xtp_to_kf_order_id_.end()) {
    if (not config_.sync_external_order) {
      SPDLOG_ERROR("unrecognized order_xtp_id {}@{}", order_info.order_xtp_id, trading_day_);
      return false;
    }
    auto writer = get_writer(location::PUBLIC);
    auto nano = yijinjing::time::now_in_nano();
    Order &order = writer->open_data<Order>(now());
    order.order_id = writer->current_frame_uid();
    from_xtp(order_info, order);
    order.insert_time = nano;
    order.update_time = nano;
    writer->close_data();
    SPDLOG_DEBUG("Order: {}", order.to_string());
    orders_.emplace(order.uid(), state<Order>(get_home_uid(), location::PUBLIC, nano, order));
    map_kf_to_xtp_order_id_.emplace(order.order_id, order_info.order_xtp_id);
    map_xtp_to_kf_order_id_.emplace(order_info.order_xtp_id, order.order_id);
    try_deal_XTPTradeReport(order_info.order_xtp_id);
    return true;
  }

  auto is_error = error_info.error_id != 0;
  auto order_id = map_xtp_to_kf_order_id_.at(order_info.order_xtp_id);
  auto &order_state = orders_.at(order_id);
  if (not has_writer(order_state.dest)) {
    SPDLOG_DEBUG("order dest: {} is not live, do not write data", get_vendor().get_location_uname(order_state.dest));
    return false;
  }

  auto writer = get_writer(order_state.dest);
  from_xtp(order_info, order_state.data);
  order_state.data.update_time = yijinjing::time::now_in_nano();
  if (is_error) {
    order_state.data.error_id = error_info.error_id;
    strncpy(order_state.data.error_msg, error_info.error_msg, ERROR_MSG_LEN);
  }
  writer->write(now(), order_state.data);
  SPDLOG_DEBUG("Order: {}", order_state.data.to_string());
  try_deal_XTPTradeReport(order_info.order_xtp_id);
  return true;
}

bool TraderXTP::custom_OnOrderEvent(const event_ptr &event) {
  const auto *bf_order_info = reinterpret_cast<const buffer_XTPOrderInfo *>(event->data_address());
  return custom_OnOrderEvent(bf_order_info->order_info, bf_order_info->error_info, bf_order_info->session_id);
}

void TraderXTP::OnTradeEvent(XTPTradeReport *trade_info, uint64_t session_id) {
  if (nullptr == trade_info) {
    SPDLOG_ERROR("XTPTradeReport is nullptr");
    return;
  }
  SPDLOG_DEBUG("XTPTradeReport: {}", to_string(*trade_info));

  auto frame = get_thread_writer()->open_frame(now(), kXTPTradeReportType_, sizeof(buffer_XTPOrderInfo));
  auto *bf_trade_info =
      const_cast<buffer_XTPTradeReport *>(reinterpret_cast<const buffer_XTPTradeReport *>(frame->data_address()));
  memcpy(&bf_trade_info->trade_info, trade_info, sizeof(XTPTradeReport));
  bf_trade_info->session_id = session_id;
  get_thread_writer()->close_frame(sizeof(buffer_XTPOrderInfo));
}

bool TraderXTP::custom_OnTradeEvent(const XTPTradeReport &trade_info, uint64_t session_id) {
  if (map_xtp_to_kf_order_id_.find(trade_info.order_xtp_id) == map_xtp_to_kf_order_id_.end()) {
    SPDLOG_ERROR("unrecognized order_xtp_id {}, store in map_xtp_order_id_to_XTPTradeReports_",
                 trade_info.order_xtp_id);
    map_xtp_order_id_to_XTPTradeReports_.try_emplace(trade_info.order_xtp_id).first->second.push_back(trade_info);
    return false;
  }

  std::set<std::string> &exec_ids =
      map_xtp_order_id_to_xtp_trader_ids_.try_emplace(trade_info.order_xtp_id).first->second;
  if (exec_ids.find(trade_info.exec_id) != exec_ids.end()) {
    SPDLOG_DEBUG("已经处理过的成交. order_xtp_id:{}, exec_id: {}, 不再重新处理", trade_info.order_xtp_id,
                 trade_info.exec_id);
    return false;
  }

  auto order_id = map_xtp_to_kf_order_id_.at(trade_info.order_xtp_id);
  auto &order_state = orders_.at(order_id);
  if (not has_writer(order_state.dest)) {
    SPDLOG_DEBUG("order dest: {} is not live, do not write data", get_vendor().get_location_uname(order_state.dest));
    return false;
  }

  exec_ids.emplace(trade_info.exec_id);
  auto writer = get_writer(order_state.dest);
  Trade &trade = writer->open_data<Trade>(now());
  from_xtp(trade_info, trade);
  trade.trade_id = writer->current_frame_uid();
  trade.order_id = order_id;
  trade.trade_time = yijinjing::time::now_in_nano();
  strcpy(trade.trading_day, trading_day_.c_str());
  trade.instrument_type = get_instrument_type(trade.exchange_id, trade.instrument_id);
  writer->close_data();

//  if (not is_final_status(order_state.data.status)) {
//    order_state.data.volume_left -= trade.volume;
//    if (order_state.data.volume_left > 0) {
//      order_state.data.status = OrderStatus::PartialFilledActive;
//    }
//    order_state.data.update_time = now();
//    writer->write(now(), order_state.data);
//  }
  return true;
}

bool TraderXTP::custom_OnTradeEvent(const event_ptr &event) {
  SPDLOG_DEBUG("msg_type: {}", event->msg_type());
  const auto *bf_trade_info = reinterpret_cast<const buffer_XTPTradeReport *>(event->data_address());
  return custom_OnTradeEvent(bf_trade_info->trade_info, bf_trade_info->session_id);
}

void TraderXTP::OnCancelOrderError(XTPOrderCancelInfo *cancel_info, XTPRI *error_info, uint64_t session_id) {
  if (nullptr == cancel_info) {
    SPDLOG_ERROR("XTPOrderCancelInfo is nullptr");
    return;
  }
  if (nullptr == error_info) {
    SPDLOG_ERROR("XTPRI is nullptr");
    return;
  }
  SPDLOG_ERROR("cancel order error, cancel_info: {}, error_id: {}, error_msg: {}, session_id: {}",
               to_string(*cancel_info), error_info->error_id, error_info->error_msg, session_id);
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

  SPDLOG_INFO("OnQueryPosition: {}", to_string(*position));
  auto writer = get_position_writer();
  Position &stock_pos = writer->open_data<Position>(0);
  if (error_info == nullptr || error_info->error_id == 0) {
    from_xtp(*position, stock_pos);
  }
  stock_pos.holder_uid = get_home()->uid;
  stock_pos.instrument_type = get_instrument_type(stock_pos.exchange_id, stock_pos.instrument_id);
  stock_pos.direction = Direction::Long;
  strncpy(stock_pos.trading_day, this->trading_day_.c_str(), DATE_LEN);
  stock_pos.update_time = yijinjing::time::now_in_nano();
  writer->close_data();
  if (is_last) {
    PositionEnd &end = writer->open_data<PositionEnd>(0);
    end.holder_uid = get_home()->uid;
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
    SPDLOG_INFO("OnQueryAsset: {}", to_string(*asset));
    auto writer = get_asset_writer();
    Asset &account = writer->open_data<Asset>(0);
    if (error_info == nullptr || error_info->error_id == 0) {
      from_xtp(*asset, account);
    }
    strncpy(account.trading_day, this->trading_day_.c_str(), DATE_LEN);
    account.holder_uid = get_home()->uid;
    account.update_time = yijinjing::time::now_in_nano();
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

// void TraderXTP::OnQueryOrder(XTPQueryOrderRsp *order_info, XTPRI *error_info, int request_id, bool is_last,
//                              uint64_t session_id) {
//   // TD重连查询收到nullptr
//   if (order_info == nullptr and map_request_location_.find(request_id) == map_request_location_.end()) {
//     SPDLOG_ERROR("XTPQueryOrderRsp is nullptr");
//     return;
//   }
//
//   // 查询历史流水收到nullptr
//   if (order_info == nullptr and map_request_location_.find(request_id) != map_request_location_.end()) {
//     SPDLOG_WARN("XTPQueryOrderRsp* order_info == nullptr, no data returned!");
//     auto writer = get_history_writer(request_id);
//     HistoryOrder &history_order = writer->open_data<HistoryOrder>();
//     history_order.is_last = true;
//     strncpy(history_order.error_msg, "返回数据为空, 可能代表无历史Order数据", ERROR_MSG_LEN);
//     writer->close_data();
//     return;
//   }
//
//   SPDLOG_DEBUG("XTPQueryOrderRsp: {}", to_string(*order_info));
//   if (map_request_location_.find(request_id) == map_request_location_.end()) {
//     // TD重连收到推送当做普通下单委托响应处理
//     return OnOrderEvent(order_info, error_info, request_id);
//   }
//
//   auto writer = get_history_writer(request_id);
//   HistoryOrder &history_order = writer->open_data<HistoryOrder>();
//
//   auto is_error = error_info != nullptr and error_info->error_id != 0;
//   if (is_error) {
//     SPDLOG_ERROR("OnQueryOrder False , error_code : {}, error_msg : {}", error_info->error_id,
//     error_info->error_msg); history_order.error_id = error_info->error_id; strncpy(history_order.error_msg,
//     error_info->error_msg, ERROR_MSG_LEN);
//   }
//
//   strncpy(history_order.trading_day, trading_day_.c_str(), DATE_LEN);
//   from_xtp(*order_info, history_order);
//   history_order.order_id = writer->current_frame_uid();
//   history_order.is_last = is_last;
//   history_order.insert_time = yijinjing::time::now_in_nano();
//   history_order.update_time = history_order.insert_time;
//   writer->close_data();
// }

yijinjing::journal::writer_ptr TraderXTP::get_history_writer(uint64_t request_id) {
  return get_writer(map_request_location_.try_emplace(request_id).first->second);
}

void TraderXTP::on_recover() {
  for (auto &pair : orders_) {
    const std::string str_external_order_id = pair.second.data.external_order_id.to_string();
    if (not str_external_order_id.empty()) {
      uint64_t order_id = pair.first;
      uint64_t order_xtp_id = std::stoull(str_external_order_id);
      map_xtp_to_kf_order_id_.emplace(order_xtp_id, order_id);
      map_kf_to_xtp_order_id_.emplace(order_id, order_xtp_id);
    }
  }
  for (auto &pair : trades_) {
    SPDLOG_DEBUG("trade: {}", pair.second.data.to_string());
    uint64_t order_xtp_id = std::stoull(pair.second.data.external_order_id);
    map_xtp_order_id_to_xtp_trader_ids_.try_emplace(order_xtp_id)
        .first->second.emplace(pair.second.data.external_trade_id.to_string());
  }
}

bool TraderXTP::req_order_trade() {
  // 只查询系统内订单
  if (not config_.sync_external_order) {
    for (auto &pair : orders_) {
      SPDLOG_DEBUG("order: {}", pair.second.data.to_string());
      if (not is_final_status(pair.second.data.status)) {
        uint64_t order_id = pair.first;
        uint64_t order_xtp_id = map_kf_to_xtp_order_id_.at(order_id);
        int ret = api_->QueryOrderByXTPID(order_xtp_id, session_id_, get_request_id());
        if (ret != 0) {
          SPDLOG_ERROR("查询上次关闭前未完成委托的 委托状态 order_id: {}, order_xtp_id: {} 失败, 错误信息为 [{}]:{}",
                       order_id, order_xtp_id, api_->GetApiLastError()->error_id, api_->GetApiLastError()->error_msg);
        }
        ret = api_->QueryTradesByXTPID(order_xtp_id, session_id_, get_request_id());
        if (ret != 0) {
          SPDLOG_ERROR("查询上次关闭前未完成委托的 成交信息 order_id: {}, order_xtp_id: {} 失败,  错误信息为 [{}]:{}",
                       order_id, order_xtp_id, api_->GetApiLastError()->error_id, api_->GetApiLastError()->error_msg);
        }
      }
    }
  }

  // 查询账户下所有订单
  if (config_.sync_external_order) {
    XTPQueryOrderReq query_order{};
    int ret = api_->QueryOrders(&query_order, session_id_, get_request_id());
    if (0 != ret) {
      SPDLOG_ERROR("QueryOrders False: {}, error: {}", ret, to_string(*api_->GetApiLastError()));
    }
    XTPQueryTraderReq query_trade{};
    ret = api_->QueryTrades(&query_trade, session_id_, get_request_id());
    if (0 != ret) {
      SPDLOG_ERROR("QueryTrades False ： {}, error: {}", ret, to_string(*api_->GetApiLastError()));
    }
  }

  return true;
}

bool TraderXTP::on_custom_event(const event_ptr &event) {
  SPDLOG_DEBUG("msg_type: {}", event->msg_type());
  switch (event->msg_type()) {
  case kXTPOrderInfoType_:
    return custom_OnOrderEvent(event);
  case kXTPTradeReportType_:
    return custom_OnTradeEvent(event);
  case kQueryXTPOrderInfoType_:
    return custom_OnQueryOrder(event);
  case kQueryXTPTradeReportType_:
    return custom_OnQueryTrade(event);
  }
  return Trader::on_custom_event(event);
}

void TraderXTP::OnQueryOrder(XTPOrderInfo *order_info, XTPRI *error_info, int request_id, bool is_last,
                             uint64_t session_id) {
  auto frame = get_thread_writer()->open_frame(now(), kQueryXTPOrderInfoType_, sizeof(buffer_XTPOrderInfo));
  auto *bf_order_info =
      const_cast<buffer_XTPOrderInfo *>(reinterpret_cast<const buffer_XTPOrderInfo *>(frame->data_address()));
  if (order_info != nullptr) {
    memcpy(&bf_order_info->order_info, order_info, sizeof(XTPOrderInfo));
  } else {
    memset(&bf_order_info->order_info, 0, sizeof(XTPOrderInfo));
  }
  if (error_info != nullptr) {
    memcpy(&bf_order_info->error_info, error_info, sizeof(XTPRI));
  } else {
    memset(&bf_order_info->error_info, 0, sizeof(XTPRI));
  }
  bf_order_info->session_id = session_id;
  bf_order_info->request_id = request_id;
  bf_order_info->is_last = is_last;
  get_thread_writer()->close_frame(sizeof(buffer_XTPOrderInfo));
  SPDLOG_DEBUG("buffer_XTPOrderInfo: {}", to_string(*bf_order_info));
}

bool TraderXTP::custom_OnQueryOrder(const event_ptr &event) {
  const auto *bf_order_info = reinterpret_cast<const buffer_XTPOrderInfo *>(event->data_address());
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
    strncpy(history_order.error_msg, "返回数据为空, 可能代表无历史Order数据", ERROR_MSG_LEN);
    writer->close_data();
    SPDLOG_DEBUG("HistoryOrder: {}", history_order.to_string());
    return false;
  }

  if (map_request_location_.find(request_id) == map_request_location_.end()) {
    // TD重连收到推送当做普通下单委托响应处理
    return custom_OnOrderEvent(order_info, error_info, request_id);
  }

  auto writer = get_history_writer(request_id);
  HistoryOrder &history_order = writer->open_data<HistoryOrder>();

  auto is_error = error_info.error_id != 0;
  if (is_error) {
    SPDLOG_ERROR("OnQueryOrder False , error_code : {}, error_msg : {}", error_info.error_id, error_info.error_msg);
    history_order.error_id = error_info.error_id;
    strncpy(history_order.error_msg, error_info.error_msg, ERROR_MSG_LEN);
  }

  strncpy(history_order.trading_day, trading_day_.c_str(), DATE_LEN);
  from_xtp(order_info, history_order);
  history_order.order_id = writer->current_frame_uid();
  history_order.is_last = is_last;
  history_order.insert_time = yijinjing::time::now_in_nano();
  history_order.update_time = history_order.insert_time;
  writer->close_data();
  SPDLOG_DEBUG("HistoryOrder: {}", history_order.to_string());
  return true;
}

void TraderXTP::OnQueryTrade(XTPQueryTradeRsp *trade_info, XTPRI *error_info, int request_id, bool is_last,
                             uint64_t session_id) {
  auto frame = get_thread_writer()->open_frame(now(), kQueryXTPTradeReportType_, sizeof(buffer_XTPTradeReport));
  auto *bf_trade_info =
      const_cast<buffer_XTPTradeReport *>(reinterpret_cast<const buffer_XTPTradeReport *>(frame->data_address()));
  if (trade_info != nullptr) {
    memcpy(&bf_trade_info->trade_info, trade_info, sizeof(XTPOrderInfo));
  } else {
    memset(&bf_trade_info->trade_info, 0, sizeof(XTPOrderInfo));
  }
  if (error_info != nullptr) {
    memcpy(&bf_trade_info->error_info, error_info, sizeof(XTPRI));
  } else {
    memset(&bf_trade_info->error_info, 0, sizeof(XTPRI));
  }
  bf_trade_info->session_id = session_id;
  bf_trade_info->request_id = request_id;
  bf_trade_info->is_last = is_last;
  get_thread_writer()->close_frame(sizeof(buffer_XTPOrderInfo));
  SPDLOG_DEBUG("buffer_XTPTradeReport: {}", to_string(*bf_trade_info));
}

bool TraderXTP::custom_OnQueryTrade(const event_ptr &event) {
  const auto *bf_trade_info = reinterpret_cast<const buffer_XTPTradeReport *>(event->data_address());
  return custom_OnQueryTrade(bf_trade_info->trade_info, bf_trade_info->error_info, bf_trade_info->request_id,
                             bf_trade_info->is_last, bf_trade_info->session_id);
}

bool TraderXTP::custom_OnQueryTrade(const XTPTradeReport &trade_info, const XTPRI &error_info, int request_id,
                                    bool is_last, uint64_t session_id) {
  SPDLOG_DEBUG("XTPTradeReport: {}", to_string(trade_info));
  SPDLOG_DEBUG("XTPRI: {}", to_string(error_info));
  SPDLOG_DEBUG("request_id: {}, is_last: {}", request_id, is_last);

  // 查询历史流水收到nullptr
  if (trade_info.order_xtp_id == 0 and is_last and
      map_request_location_.find(request_id) != map_request_location_.end()) {
    SPDLOG_WARN("XTPQueryTradeRsp* trade_info == nullptr, no data returned!");
    auto writer = get_history_writer(request_id);
    HistoryTrade &history_trade = writer->open_data<HistoryTrade>(now());
    history_trade.is_last = true;
    strncpy(history_trade.error_msg, "返回数据为空,可能代表无历史Trade数据", ERROR_MSG_LEN);
    writer->close_data();
    return false;
  }

  if (map_request_location_.find(request_id) == map_request_location_.end()) {
    // TD重连收到推送当做普通交易成交回报推送处理
    return custom_OnTradeEvent(trade_info, session_id);
  }

  auto writer = get_history_writer(request_id);
  HistoryTrade &history_trade = writer->open_data<HistoryTrade>(now());

  auto is_error = error_info.error_id != 0;
  if (is_error) {
    SPDLOG_ERROR("OnQueryTrade False , error_code : {}, error_msg : {}", error_info.error_id, error_info.error_msg);
    history_trade.error_id = error_info.error_id;
    strncpy(history_trade.error_msg, error_info.error_msg, ERROR_MSG_LEN);
  }

  from_xtp(trade_info, history_trade);
  history_trade.trade_id = writer->current_frame_uid();
  history_trade.is_last = is_last;
  history_trade.trade_time = yijinjing::time::now_in_nano();
  strncpy(history_trade.trading_day, trading_day_.c_str(), DATE_LEN);
  history_trade.instrument_type = get_instrument_type(history_trade.exchange_id, history_trade.instrument_id);
  writer->close_data();
  SPDLOG_DEBUG("HistoryTrade: {}", history_trade.to_string());
  return false;
}

void TraderXTP::try_deal_XTPTradeReport(uint64_t xtp_order_id) {
  auto &xtp_trades = map_xtp_order_id_to_XTPTradeReports_.try_emplace(xtp_order_id).first->second;
  for (const auto &xtp_trade : xtp_trades) {
    custom_OnTradeEvent(xtp_trade, session_id_);
  }
  xtp_trades.clear();
}

} // namespace kungfu::wingchun::xtp
