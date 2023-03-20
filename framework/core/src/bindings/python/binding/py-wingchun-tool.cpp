// SPDX-License-Identifier: Apache-2.0

#include "py-wingchun.h"

#include <kungfu/longfist/longfist.h>
#include <kungfu/wingchun/tool/cachetool.h>

using namespace kungfu::longfist;
using namespace kungfu::longfist::enums;
using namespace kungfu::longfist::types;
using namespace kungfu::yijinjing::data;
using namespace kungfu::wingchun::tool;

namespace py = pybind11;

namespace kungfu::wingchun::pybind {
void bind_tool(pybind11::module &m) {
  py::class_<CacheTool, std::shared_ptr<CacheTool>>(m, "CacheTool")
      .def(py::init<category, std::string, std::string, std::string, std::string, locator_ptr, bool>(),
           py::arg("category"), py::arg("group"), py::arg("name"), py::arg("start_time"), py::arg("end_time"),
           py::arg("locator"), py::arg("overwrite") = true)
      .def(py::init<category, std::string, std::string, int64_t, int64_t, locator_ptr, bool>(), py::arg("category"),
           py::arg("group"), py::arg("name"), py::arg("start_time"), py::arg("end_time"), py::arg("locator"),
           py::arg("overwrite") = true)
      .def("get_begin_time", &CacheTool::get_begin_time)
      .def("get_end_time", &CacheTool::get_end_time)
      .def("get_location", &CacheTool::get_location)
      .def("run", &CacheTool::run);

  auto cache_tool_class =
      py::class_<CacheToolWriter, CacheTool, std::shared_ptr<CacheToolWriter>>(m, "CacheToolWriter")
          .def(py::init<category, std::string, std::string, std::string, std::string, locator_ptr>(),
               py::arg("category"), py::arg("group"), py::arg("name"), py::arg("start_time"), py::arg("end_time"),
               py::arg("locator"));

  boost::hana::for_each(AllDataTypes, [&](auto type) {
    using DataType = typename decltype(+boost::hana::second(type))::type;
    cache_tool_class.def(
        "write_at",
        py::overload_cast<int64_t, int64_t, uint32_t, const DataType &>(&CacheToolWriter::write_at<DataType>),
        py::arg("gen_time"), py::arg("trigger_time"), py::arg("dest_id"), py::arg("data"));
  });

  py::class_<CacheToolReader, CacheTool, std::shared_ptr<CacheToolReader>>(m, "CacheToolReader")
      .def(py::init<category, std::string, std::string, std::string, std::string, locator_ptr>(), py::arg("category"),
           py::arg("group"), py::arg("name"), py::arg("start_time"), py::arg("end_time"), py::arg("locator"))
      .def("current_frame", &CacheToolReader::current_frame)
      .def("next", &CacheToolReader::next)
      .def("data_available", &CacheToolReader::data_available)
      .def("join", &CacheToolReader::join);
}
} // namespace kungfu::wingchun::pybind