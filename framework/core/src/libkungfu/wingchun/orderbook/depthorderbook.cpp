#include <kungfu/wingchun/orderbook/depthorderbook.h>

namespace kungfu::wingchun::orderbook {
DepthOrderbook::DepthOrderbook() {}
orderbook::OrderbookSide DepthOrderbook::get_bids(std::string instrument_id, std::string exchange_id) {
  const std::string unique_id = fmt::format("{}:{}", exchange_id, instrument_id);
  std::map<std::string, std::map<int, Level>> bid_map = getBidMap();
  if (bid_map.find(unique_id) == bid_map.end()) {
    return orderbook::OrderbookSide();
  }
  return orderbook::OrderbookSide(bid_map.at(unique_id));
}

orderbook::OrderbookSide DepthOrderbook::get_asks(std::string instrument_id, std::string exchange_id) {
  const std::string unique_id = fmt::format("{}:{}", exchange_id, instrument_id);
  std::map<std::string, std::map<int, Level>> ask_map = getAskMap();
  if (ask_map.find(unique_id) == ask_map.end()) {
    return orderbook::OrderbookSide();
  }
  return orderbook::OrderbookSide(ask_map.at(unique_id));
}

void DepthOrderbook::on_entrust(const event_ptr &event) {
  // 实现 on_entrust 函数，维护该标的的 orderbook
  SPDLOG_INFO("测试 on_entrust");
}

void DepthOrderbook::on_transaction(const event_ptr &event) {
  // 实现 on_transaction 函数，维护该标的的 orderbook
  SPDLOG_INFO("测试 on_transaction");
}
} // namespace kungfu::wingchun::orderbook
