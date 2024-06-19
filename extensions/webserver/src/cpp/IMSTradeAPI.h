#pragma once
#include "kungfu_types.h"
#include <kungfu/common.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/journal/journal.h>
#include <kungfu/yijinjing/nanomsg/webserver.h>

#include <string>

namespace CICC {

namespace API {

// using namespace types;

class IMSTradeSPI {
public:
  IMSTradeSPI(){};
  ~IMSTradeSPI(){};
  virtual void OnRspNewOrder(types::OrderInput *input);
  virtual void OnRspCancelOrder(types::OrderAction *action);
  virtual void OnNotiOrder(types::Order *order);
  virtual void OnNotiKnock(types::Trade *trade);
};

class IMSTradeAPI {
public:
  static IMSTradeAPI *CreateApi();
  static void ReleaseAPI(IMSTradeAPI *pApi);
  virtual bool SetTradeSPI(IMSTradeSPI *pSpi);
  virtual bool Initial(const char *addr);
  virtual bool TryLogin(const char *account_group, const char *account_name, const uint8_t &method);
  virtual const char *GetLastError();

  virtual int ReqNewOrder(types::OrderInput &order);
  virtual int ReqCancelOrder(types::OrderAction &order);

private:
  void data_recv();
  int ReqDataRound(int limit = 100);

private:
  IMSTradeAPI();
  ~IMSTradeAPI();
  kungfu::yijinjing::webserver::stream_manage_ptr stream_manager_;
  kungfu::yijinjing::webserver::websocket_client_ptr client_;
  kungfu::yijinjing::journal::reader_ptr reader_ = nullptr;
  IMSTradeSPI *spi_;
  std::thread *thread_;
};

} // namespace API
} // namespace CICC
