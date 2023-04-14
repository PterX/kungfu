// SPDX-License-Identifier: Apache-2.0

//
// Created by Keren Dong on 2020/2/15.
//

#ifndef KUNGFU_NODE_LONGFIST_H
#define KUNGFU_NODE_LONGFIST_H

#include "common.h"
#include "operators.h"

namespace kungfu::node {
class Longfist : public Napi::ObjectWrap<Longfist> {
public:
  explicit Longfist(const Napi::CallbackInfo &info);

  static void Init(Napi::Env env, Napi::Object exports);

  Napi::Value GetMsgTypes(const Napi::CallbackInfo &info);

  void InitMsgTypes(const Napi::CallbackInfo &info);

  Napi::Value GetTypes(const Napi::CallbackInfo &info);

  void InitTypes(const Napi::CallbackInfo &info);

  void NoSet(const Napi::CallbackInfo &info, const Napi::Value &value);

private:
  static Napi::FunctionReference constructor;

  Napi::ObjectReference types_ref_;
  Napi::ObjectReference msg_types_ref_;
};
} // namespace kungfu::node

#endif // KUNGFU_NODE_LONGFIST_H
