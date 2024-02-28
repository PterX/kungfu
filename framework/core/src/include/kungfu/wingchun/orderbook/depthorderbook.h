#ifndef DEPTHORDERBOOK_H
#define DEPTHORDERBOOK_H

#include <kungfu/wingchun/orderbook/orderbook.h>

namespace kungfu::wingchun::orderbook {
class DepthOrderbook : public orderbook::Orderbook {
public:
  explicit DepthOrderbook();

  // 覆盖基类的虚函数
  orderbook::OrderbookSide get_bids(std::string instrument_id, std::string exchange_id) override;

  orderbook::OrderbookSide get_asks(std::string instrument_id, std::string exchange_id) override;

  void on_entrust(const event_ptr &event) override;

  void on_transaction(const event_ptr &event) override;

private:
  // 其他成员变量或函数声明
};
} // namespace kungfu::wingchun::orderbook

#endif // DEPTHORDERBOOK_H
