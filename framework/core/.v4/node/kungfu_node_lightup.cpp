// SPDX-License-Identifier: Apache-2.0
//
// Kungfu 4.0 Step 3 最小点亮：精简版 kungfu_node N-API addon。
// 只注册 longfist 类型 + journal 读取(Frame/Reader/Assemble/IODevice)，
// 跳过 watcher / 各交易 store / tracer / sqlite。证明 Node.js 能读到
// C++/Python 经同一条 yijinjing journal 写入的同一帧。
//
// 注意：现有 Node 绑定的 Reader 构造写死 GetDefaultRuntimeLocator()，即只认
// 环境变量 KF_RUNTIME_DIR(默认 ~/.../runtime)，不认 openReader 所属 io_device
// 的 base。故读自定义目录的 journal 时须设 KF_RUNTIME_DIR 指向该目录。
#include <napi.h>

#include "io.h"
#include "journal.h"
#include "longfist.h"

namespace kungfu::node {

Napi::Object InitLightup(Napi::Env env, Napi::Object exports) {
  Longfist::Init(env, exports);
  Frame::Init(env, exports);
  Reader::Init(env, exports);
  Assemble::Init(env, exports);
  IODevice::Init(env, exports);
  return exports;
}

} // namespace kungfu::node

// NODE_API_MODULE 对 regfunc 做 token 粘贴(__napi_##regfunc)，故须传未限定标识符。
using kungfu::node::InitLightup;
NODE_API_MODULE(kungfu_node, InitLightup)
