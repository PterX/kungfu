//
// Created by Fuxk on 2023/4/17.
//

#ifndef XTP_BUFFER_DATA_H
#define XTP_BUFFER_DATA_H

#endif // XTP_BUFFER_DATA_H

#include <xtp_trader_api.h>

struct buffer_XTPOrderInfo {
  XTPOrderInfo order_info;
  uint64_t session_id;
  XTPRI error_info;
};

struct buffer_XTPTradeReport {
  XTPTradeReport trade_info;
  uint64_t session_id;
};