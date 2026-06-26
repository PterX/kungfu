// SPDX-License-Identifier: Apache-2.0

#include "py-wingchun.h"

namespace kungfu::wingchun {
void bind(pybind11::module &&m) {
  // tracing-foundation Phase 1: 仅保留 header-only 的 instrument utils(common.h inline),
  // 供 Python 层 kungfu/wingchun 使用;交易语义绑定(map/book/orderbook/stream/factor/
  // broker/service/strategy/operator/tool)的源已从编译流程排除,不在此注册。
  pybind::bind_utils(m);
}
} // namespace kungfu::wingchun
