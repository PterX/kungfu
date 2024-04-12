// SPDX-License-Identifier: Apache-2.0

#ifndef WINGCHUN_STREAMDATABATCHER_H
#define WINGCHUN_STREAMDATABATCHER_H

#include <kungfu/longfist/enums.h>
#include <kungfu/longfist/types.h>
#include <kungfu/wingchun/broker/client.h>
#include <kungfu/yijinjing/common.h>
#include <kungfu/yijinjing/practice/apprentice.h>
#include <kungfu/wingchun/orderbook/orderbooks.h>

namespace kungfu::wingchun::streamdatabatcher {

class StreamDataBatcher {
public:
  // StreamDataBatcher() = default;
  StreamDataBatcher() {
    longfist::types::Entrust entrust1;
    longfist::types::Entrust entrust2;
    entrust1.instrument_id = "600000";
    entrust2.instrument_id = "330059";
    entrust1.biz_index = 600000;
    entrust2.biz_index = 300059;
    entrust1.price = 1.1;
    entrust2.price = 2.2;
    entrust1.Side = kungfu::longfist::enums::Side::Buy;
    entrust2.Side = kungfu::longfist::enums::Side::Sell;
    entrust1.instrument_type = kungfu::longfist::enums::InstrumentType::Stock;
    entrust2.instrument_type = kungfu::longfist::enums::InstrumentType::Future;
    entrust_vec_.push_back(entrust1);
    entrust_vec_.push_back(entrust2);
  }
  virtual ~StreamDataBatcher() = default;

  // todo 构造函数需要接收SliceIndexer 回测模式就用这个去找journal 实盘不用 后面加

  // todo pop batched data_from 清空队列的函数？ 暂时没搞清楚使用场景 后面加

  // 接受数据流 按时间窗口将数据流分批输出 todo 暂时写成这样 后面再改
  // 1 pybind 调用细节
  // 2 来数据 向缓冲区添加事件 更新map<unique_id, vector> 调用pop_batched(unique_id) 清空map和缓冲区

  std::vector<longfist::types::Entrust> &get_entrust_events() { return entrust_vec_; }

private:
  std::vector<longfist::types::Entrust> entrust_vec_;
};

} // namespace kungfu::wingchun::streamdatabatcher
#endif // WINGCHUN_STREAMDATABATCHER_H