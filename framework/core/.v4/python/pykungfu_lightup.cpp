// SPDX-License-Identifier: Apache-2.0
//
// Kungfu 4.0 最小点亮：精简版 pykungfu，只绑 longfist + yijinjing(journal)，
// 跳过 libnode(Step 3) 与 wingchun。证明 Python 能读写 C++ 的 journal/longfist。
#include "py-longfist.h"
#include "py-yijinjing.h"

#include <kungfu/yijinjing/common.h>

namespace py = pybind11;

namespace kungfu::yijinjing {
// 最小点亮用的具体 publisher：journal writer 的 close_frame 末尾会无条件调
// publisher_->notify()，单进程写读 demo 无 master，故用 noop 实现全部纯虚。
// 注意 publisher 间接继承 resource(is_usable/setup 也是纯虚)，必须一并实现，
// 否则类仍是抽象的、py::init<>() 也构造不出来。
class noop_publisher : public publisher {
public:
  bool is_usable() override { return true; }
  bool setup() override { return true; }
  int notify() override { return 0; }
  int publish(const std::string &, int, bool) override { return 0; }
};
} // namespace kungfu::yijinjing

PYBIND11_MODULE(pykungfu, m) {
  kungfu::longfist::bind(m.def_submodule("longfist"));
  auto yjj = m.def_submodule("yijinjing");
  kungfu::yijinjing::bind(std::move(yjj));
  // 在 yijinjing 子模块上补一个可直接构造的 noop publisher。
  py::class_<kungfu::yijinjing::noop_publisher, kungfu::yijinjing::publisher,
             std::shared_ptr<kungfu::yijinjing::noop_publisher>>(yjj, "noop_publisher")
      .def(py::init<>());
}
