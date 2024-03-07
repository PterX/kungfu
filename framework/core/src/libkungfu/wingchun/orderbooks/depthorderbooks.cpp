#include <kungfu/wingchun/orderbooks/depthorderbooks.h>

namespace kungfu::wingchun::orderbook {
OrderbookSide& DepthOrderbooks::get_bids(std::string instrument_id, std::string exchange_id) {
  const std::string unique_id = fmt::format("{}:{}", exchange_id, instrument_id);
  SPDLOG_INFO("测试 get_bids unique_id: {}", unique_id);
  return bid_sides_.try_emplace(unique_id).first->second;
}

OrderbookSide& DepthOrderbooks::get_asks(std::string instrument_id, std::string exchange_id) {
  const std::string unique_id = fmt::format("{}:{}", exchange_id, instrument_id);
  SPDLOG_INFO("测试 get_asks unique_id: {}", unique_id);
  return ask_sides_.try_emplace(unique_id).first->second;
}

void DepthOrderbooks::on_entrust(const event_ptr &event) {
  // side 
  // bid_sides_.try_emplace(unique_id).first->second.on_entrust(event);
  // ask_sides_
  // 实现 on_entrust 函数，维护该标的的 orderbook
  // 撮合 维护level
  // 跨天 if () { }
  SPDLOG_INFO("测试 on_entrust");
  std::string instrument_id = event->data<Entrust>().instrument_id;
  std::string exchange_id = event->data<Entrust>().exchange_id;
  const std::string unique_id = fmt::format("{}:{}", exchange_id, instrument_id);
  Side side = event->data<Entrust>().side;
  double price = event->data<Entrust>().price;
  int64_t volume = event->data<Entrust>().volume;
  int64_t data_time = event->data<Entrust>().data_time;
  SPDLOG_INFO("测试 Entrust : {}", event->data<Entrust>().to_string());

  Level level = Level(price, volume, data_time);
  // if (side == Side::Buy) {
  //   if (bid_map.find(unique_id) != bid_map.end()) {
  //     // 如果存过当前标的的买单数据 修改当前数据
  //     if (bid_map[unique_id].find(price) != bid_map[unique_id].end()) {
  //       // 如果该标的已经有当前价格的level数据 就增加买单的volume 更新时间
  //       bid_map[unique_id][price].volume += volume;
  //       bid_map[unique_id][price].data_time = data_time;
  //     } else {
  //       // 如果该标的没有存过当前价格 就建立新的键值对
  //       bid_map[unique_id][price] = level;
  //     }
  //   } else {
  //     // 没有存过当前标的的数据 存入当前数据
  //     std::map<double, Level, std::greater<double>> temp_map;
  //     temp_map[price] = level;
  //     bid_map[unique_id] = temp_map;
  //   }
  // } else if (side == Side::Sell) {
  //   if (ask_map.find(unique_id) != ask_map.end()) {
  //     // 如果存过当前标的的买单数据 修改当前数据
  //     if (ask_map[unique_id].find(price) != ask_map[unique_id].end()) {
  //       // 如果该标的已经有当前价格的level数据 就增加卖单的volume 更新时间
  //       ask_map[unique_id][price].volume += volume;
  //       ask_map[unique_id][price].data_time = data_time;
  //     } else {
  //       // 如果该标的没有存过当前价格 就建立新的键值对
  //       ask_map[unique_id][price] = level;
  //     }
  //   } else {
  //     // 没有存过当前标的的数据 存入当前数据
  //     std::map<double, Level> temp_map;
  //     temp_map[price] = level;
  //     ask_map[unique_id] = temp_map;
  //   }
  // }
}

void DepthOrderbooks::on_transaction(const event_ptr &event) {
  // // 实现 on_transaction 函数，维护该标的的 orderbook
  // SPDLOG_INFO("测试 on_transaction");
  // std::string instrument_id = event->data<Transaction>().instrument_id;
  // std::string exchange_id = event->data<Transaction>().exchange_id;
  // const std::string unique_id = fmt::format("{}:{}", exchange_id, instrument_id);
  // Side side = event->data<Transaction>().side;
  // double price = event->data<Transaction>().price;
  // int64_t volume = event->data<Transaction>().volume;
  // int64_t data_time = event->data<Transaction>().data_time;
  // SPDLOG_INFO("测试 Transaction : {}", event->data<Transaction>().to_string());

  // Level level = Level(price, volume, data_time);
  // if (side == Side::Buy) {
  //   if (bid_map.find(unique_id) != bid_map.end()) {
  //     // 如果存过当前标的的买单数据 修改当前数据
  //     if (bid_map[unique_id].find(price) != bid_map[unique_id].end()) {
  //       // 如果该标的已经有当前价格的level数据 就减少买单的volume 更新时间
  //       // todo 需要判断当前价格剩余的volume 是否大于成交单的volume
  //       bid_map[unique_id][price].volume -= volume;
  //       bid_map[unique_id][price].data_time = data_time;
  //     } else {
  //       // 如果该标的没有存过当前价格 就建立新的键值对
  //       bid_map[unique_id][price] = level;
  //     }
  //   } else {
  //     // 没有存过当前标的的数据 存入当前数据
  //     std::map<double, Level, std::greater<double>> temp_map;
  //     temp_map[price] = level;
  //     bid_map[unique_id] = temp_map;
  //   }
  // } else if (side == Side::Sell) {
  //   if (ask_map.find(unique_id) != ask_map.end()) {
  //     // 如果存过当前标的的买单数据 修改当前数据
  //     if (ask_map[unique_id].find(price) != ask_map[unique_id].end()) {
  //       // 如果该标的已经有当前价格的level数据 就增加卖单的volume 更新时间
  //       ask_map[unique_id][price].volume -= volume;
  //       ask_map[unique_id][price].data_time = data_time;
  //     } else {
  //       // 如果该标的没有存过当前价格 就建立新的键值对
  //       ask_map[unique_id][price] = level;
  //     }
  //   } else {
  //     // 没有存过当前标的的数据 存入当前数据
  //     std::map<double, Level> temp_map;
  //     temp_map[price] = level;
  //     ask_map[unique_id] = temp_map;
  //   }
  // }
}
} // namespace kungfu::wingchun::orderbook
