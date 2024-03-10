#include <kungfu/wingchun/orderbook/quoteorderbooks.h>

namespace kungfu::wingchun::orderbook {

void DepthOrderbook::on_entrust(const Entrust &quote) {
  Level level = bid_side_.levels_[1.0];
  std::map<double, Level> &bid_map = bid_side_.levels_;
  std::map<double, Level> &ask_map = ask_side_.levels_;
  // bid map, ask map 都能拿到了随便玩吧。
  

}

void DepthOrderbook::on_transaction(const Transaction &transaction) {
  Level level = bid_side_.levels_[1.0];
  std::map<double, Level> &bid_map = bid_side_.levels_;
  std::map<double, Level> &ask_map = ask_side_.levels_;
  // bid map, ask map 都能拿到了随便玩吧。
  

}


} // namespace kungfu::wingchun::orderbook
