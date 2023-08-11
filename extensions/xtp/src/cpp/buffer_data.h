//
// Created by Fuxk on 2023/4/17.
//

#ifndef XTP_BUFFER_DATA_H
#define XTP_BUFFER_DATA_H

#include "serialize_xtp.h"

static constexpr int32_t kXTPOrderInfoType = 12340001;
static constexpr int32_t kXTPTradeReportType = 12340002;
static constexpr int32_t kQueryXTPOrderInfoType = 12340003;
static constexpr int32_t kQueryXTPTradeReportType = 12340004;
static constexpr int32_t kCancelOrderErrorType = 12340005;

struct BufferXTPTradeReport {
  XTPQueryTradeRsp trade_info;
  XTPRI error_info;
  int request_id;
  bool is_last;
  uint64_t session_id;
};

struct BufferXTPOrderInfo {
  XTPOrderInfo order_info;
  XTPRI error_info;
  int request_id;
  bool is_last;
  uint64_t session_id;
};

struct BufferXTPOrderCancelInfo {
  XTPOrderCancelInfo cancel_info;
  XTPRI error_info;
  uint64_t session_id;
};

namespace nlohmann {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BufferXTPTradeReport, trade_info, session_id, error_info, request_id, is_last);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BufferXTPOrderInfo, order_info, session_id, error_info, request_id, is_last);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BufferXTPOrderCancelInfo, cancel_info, error_info, session_id);

} // namespace nlohmann

#endif // XTP_BUFFER_DATA_H