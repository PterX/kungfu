// SPDX-License-Identifier: Apache-2.0
//
// born-FB Order 写侧构造器:把 longfist POD Order 按 char-array/enum 表示口径(2026-06-21)序列化成
// born-FB Order 载荷(FB 编码),供生产 producer 在 feature flag 下 write_raw 到 journal,与 POD Order(tag 202)并存。
// 口径:array<char,N> instrument_id -> FB string(到首 NUL,对齐 sqlite_orm text_printer);
//      enum class OrderStatus:int8_t -> FB OrderStatus:byte(镜像 enums.h,值=底层 int8)。
// 读侧由 fb_projector 反射投影(R3 同一 order.fbs/.bfbs 驱 journal+SQLite)。
#ifndef KUNGFU_LONGFIST_FB_ORDER_FB_BUILDER_H
#define KUNGFU_LONGFIST_FB_ORDER_FB_BUILDER_H

#include <kungfu/longfist/fb/order_generated.h>
#include <kungfu/longfist/types.h>

#include <flatbuffers/flatbuffers.h>

#include <cstdint>
#include <string>

namespace kungfu::longfist::fb {

// born-FB Order 迁移 msg_type(>0,区分 POD Order longfist tag 202,迁移期并存)。
static constexpr int32_t ORDER_FB_TAG = 30202;

// 把 POD Order 构造成 born-FB Order 载荷;返回 FB 字节,可直接 write_raw。
inline std::string build_fb_order(const kungfu::longfist::types::Order &o) {
  flatbuffers::FlatBufferBuilder fbb;
  auto inst = fbb.CreateString(o.instrument_id.to_string()); // array<char,N> -> string(口径)
  auto root = CreateOrder(fbb, o.order_id, o.restore_time,
                          static_cast<OrderStatus>(static_cast<int8_t>(o.status)), // enums::OrderStatus -> fb::OrderStatus
                          inst, o.limit_price, o.volume, o.volume_left);
  fbb.Finish(root);
  return std::string(reinterpret_cast<const char *>(fbb.GetBufferPointer()), fbb.GetSize());
}

} // namespace kungfu::longfist::fb
#endif // KUNGFU_LONGFIST_FB_ORDER_FB_BUILDER_H
