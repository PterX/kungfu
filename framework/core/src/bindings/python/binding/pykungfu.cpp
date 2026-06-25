// SPDX-License-Identifier: Apache-2.0

#include "py-libnode.h"
#include "py-longfist.h"
#include "py-yijinjing.h"
// tracing-foundation Phase 1: wingchun(交易语义)移出编译流程,去 py-wingchun 绑定。

namespace py = pybind11;

PYBIND11_MODULE(pykungfu, m) {
  kungfu::libnode::bind(m.def_submodule("libnode"));
  kungfu::longfist::bind(m.def_submodule("longfist"));
  kungfu::yijinjing::bind(m.def_submodule("yijinjing"));
}
