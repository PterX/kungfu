// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/4/8.
//

#ifndef KUNGFU_NODE_DATA_TABLE_H
#define KUNGFU_NODE_DATA_TABLE_H

#include "common.h"
#include <kungfu/common.h>

namespace kungfu::node {
class DataTable : public Napi::ObjectWrap<DataTable> {
public:
  explicit DataTable(const Napi::CallbackInfo &info);

  ~DataTable() override = default;

  Napi::Value Filter(const Napi::CallbackInfo &info);

  Napi::Value NoFilter(const Napi::CallbackInfo &info);

  Napi::Value List(const Napi::CallbackInfo &info);

  Napi::Value Merge(const Napi::CallbackInfo &info);

  Napi::Value Range(const Napi::CallbackInfo &info);

  Napi::Value Sort(const Napi::CallbackInfo &info);

  static void Init(Napi::Env env, Napi::Object exports);

  static Napi::Value NewInstance(Napi::Value arg);

private:
  static Napi::FunctionReference constructor;
  static void cleanup() {
    SPDLOG_INFO("DataTable reset");
    DataTable::constructor.Reset();
  }
};
} // namespace kungfu::node
#endif // KUNGFU_NODE_DATA_TABLE_H
