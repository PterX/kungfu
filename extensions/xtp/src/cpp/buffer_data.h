//
// Created by Fuxk on 2023/4/17.
//

#ifndef XTP_BUFFER_DATA_H
#define XTP_BUFFER_DATA_H

#include "serialize_xtp.h"

// struct buffer_XTPOrderInfo {
//   XTPOrderInfo order_info;
//   uint64_t session_id;
//   XTPRI error_info;
// };

struct buffer_XTPTradeReport {
  XTPQueryTradeRsp trade_info;
  XTPRI error_info;
  int request_id;
  bool is_last;
  uint64_t session_id;
};

struct buffer_XTPOrderInfo {
  XTPOrderInfo order_info;
  XTPRI error_info;
  int request_id;
  bool is_last;
  uint64_t session_id;
};

namespace nlohmann {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(buffer_XTPTradeReport, trade_info, session_id);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(buffer_XTPOrderInfo, order_info, session_id, error_info, request_id, is_last);

} // namespace nlohmann

#endif // XTP_BUFFER_DATA_H