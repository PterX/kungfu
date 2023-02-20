// SPDX-License-Identifier: Apache-2.0

#include "py-wingchun.h"

#include <kungfu/wingchun/tool/cachetool.h>

using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::data;
using namespace kungfu::wingchun::tool;

namespace py = pybind11;

namespace kungfu::wingchun::pybind {
void bind_tool(pybind11::module &m) {
  py::class_<CacheTool, std::shared_ptr<CacheTool>>(m, "CacheTool")
      .def("get_begin_time", &CacheTool::get_begin_time)
      .def("get_end_time", &CacheTool::get_end_time)
      .def("run", &CacheTool::run);
}
} // namespace kungfu::wingchun::pybind