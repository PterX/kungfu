// SPDX-License-Identifier: Apache-2.0

#include "py-libnode.h"
#include "py-longfist.h"
#include "py-wingchun.h"
#include "py-yijinjing.h"
// tracing-foundation Phase 1: wingchun 交易语义移出编译流程;wingchun 子模块仅保留 header-only
// instrument utils(见 py-wingchun.cpp / py-wingchun-utils.cpp),供 Python 层 kungfu/wingchun 使用。

namespace py = pybind11;

PYBIND11_MODULE(pykungfu, m) {
  kungfu::libnode::bind(m.def_submodule("libnode"));
  kungfu::longfist::bind(m.def_submodule("longfist"));
  kungfu::wingchun::bind(m.def_submodule("wingchun"));
  kungfu::yijinjing::bind(m.def_submodule("yijinjing"));
}
