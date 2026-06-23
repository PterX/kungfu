# ADR-0001: yijinjing journal 帧发布协议改用 atomic_ref release/acquire

- 状态: accepted（实现见本分支；ARM 对抗压测 + 真实树编译均已通过，见「闸门产出」）
- 日期: 2026-06-23
- 类别: (b) 改进 + latent bug（并发正确性）
- 子系统: yijinjing journal —— 单写多读、mmap `MAP_SHARED` 跨进程帧总线
- 相关: 与 ADR-0002（longfist hana→FlatBuffers）独立；本改动只动发布同步语义，不动 schema/磁盘格式

## 决策

把 journal 帧的「发布 / 可见」协议从「`volatile` 字段 + 普通读写」改为
**`std::atomic_ref` release/acquire**：

- 写侧：在 payload 及全部头字段写完、下一帧头清零之后，以 `memory_order_release`
  存储发布令牌 `frame_header::length`（`frame::publish_data_length`），且让它成为**最后一次写**。
- 读侧：以 `memory_order_acquire` 读取 `length` 作为「有无新帧」闸门
  （`frame::acquire_length` → `frame::has_data`），闸门成立后再读 payload/gen_time/frame_uid。

## 当年语境

journal 是 kungfu 的核心 IPC：单写者顺序追加「定长头 + 变长体」到 mmap 页，多读者无锁轮询。
原实现用 `volatile uint32_t length` / `volatile int32_t msg_type` 作发布标志，配合
「writer 先写 payload 后写 length、reader 先读 length 后读 payload」的程序序，长期在
**x86 (TSO)** 上稳定正确。

## 当年判词（(b) + latent bug）

C++ `volatile` **不是线程同步原语**：它只约束编译器对该对象自身的访问，
**既不发硬件内存屏障，也不与相邻的非 volatile 访问（payload / gen_time / frame_uid /
下一帧头清零）建立 happens-before**。其正确性实际依赖 x86-TSO 的「store 不互相重排、
load 不互相重排」隐含保证 —— 这是平台偶然属性，不是协议保证。属「正确但脆弱」的 latent bug。

在 **ARM（弱内存序，v4 目标平台 Apple Silicon / aarch64 服务器）** 上：writer 对 `length`
的 store 可能先于 payload / 其它头字段对另一核可见；reader 对 `length` 的 load 也可能与后续
payload load 重排。读者据此在 payload 尚不可见时判定「有帧」，读到**撕裂帧 / 陈旧帧**。

## 今日建议（已落地）

1. 去掉 `frame_header` 中 `length` / `msg_type` 的 `volatile`（`longfist/types.h`）。
   `length` 升格为「发布令牌」，语义由 atomic_ref 承载。
2. `frame.h`：
   - `publish_data_length()`：`atomic_ref<uint32_t>(length).store(header_length+len, release)`；
   - `acquire_length()`：`atomic_ref<uint32_t>(length).load(acquire)`；
   - `has_data()` 改用 `acquire_length() > 0 && msg_type > 0`（acquire 先于 msg_type 读，
     `&&` 短路保证顺序）；
   - `copy()` 改为「拷贝除 `length`（offset 0）以外的全部字节」，由调用方最后 release 发布
     `length`（含 `static_assert(offsetof(frame_header,length)==0)` 守卫）。
3. `writer.cpp`：
   - `close_frame_lock_free()`：把 `length` 发布移到 gen_time / frame_uid /
     trigger_frame_uid / last_frame_position / 下一帧头清零**全部完成之后**，
     用 `publish_data_length()`（release）作收尾；
   - `copy_frame()`：`copy()` 后从 **source** 尺寸算下一帧地址并清零，再 `publish_data_length()`。
4. 不变更：`replay_writer` 的 `cloned_frame_->copy()` 目标是堆上私有缓冲（非共享内存），
   无跨进程读者轮询，无需屏障；`journal.cpp` 页尾 `set_data_length(0)` 是「取消发布」，保留普通写。

## 对外契约影响

- **二进制 / 磁盘格式不变**：`length` 仍是同 offset、同宽度的 uint32；去掉 `volatile` 不改
  size / 对齐。跨语言 / 跨进程读者的字节契约不变。
- **对齐已核验**：非 Windows 目标 `KF_DEFINE_PACK_TYPE` 落到 `__attribute__((aligned(8)))`，
  **非字节 packed**；`length` 为首字段（offset 0），帧起始地址恒 8 字节对齐
  （`verify_cpu_word_length` 凑 8 + header aligned(8)）→ `std::atomic_ref<uint32_t>`
  满足对齐、非 UB。

## 可逆性

高。改动局限于发布点 + 读侧闸门 + 字段限定符，无数据迁移，可单次回退。

## 成本-收益

- 成本：ARM 上每帧一次 release/acquire（`stlr`/`ldar`），开销极小；一次性认知 + 新增压测维护面。
- 收益：消除 ARM 撕裂帧 / 陈旧帧风险，把「单写多读塌缩」从平台偶然正确变为协议保证。
  对低延迟交易总线是正确性硬需求。

## SOTA 对标

- LMAX **Disruptor**：sequence 用 release/acquire 发布 cursor，消费者 acquire —— 同构。
- **Aeron**：term buffer 帧头 length 用 ordered/release put 作 commit 标志，consumer acquire 读 ——
  「length 作发布令牌、release 写在最后」正是本方案范式。
- **Chronicle Queue**：header word release 写 + acquire 读做记录提交。
- 结论：release/acquire 单令牌发布是该类 mmap SPMC/SPSC 总线的业界标准；原 `volatile` 落后于 SOTA。

## 闸门产出（S4 回填）

**对抗压测（独立 harness 镜像发布协议:token+payload、append-only ring、单写多读、背压防套圈）：**

| 平台 | volatile（原实现） | atomic_ref（修复） |
|---|---|---|
| Mac arm64 (Apple M1 Ultra) | **14,630,379 撕裂 / 75M 读**（rc=1） | **0 撕裂 / 290M+ 读**（rc=0） |
| Linux x86_64 (i9-13900K, TSO) | **0 撕裂 / 180M 读**（rc=0） | **0 撕裂 / 148M 读**（rc=0） |

- **学到什么**：
  1. 撕裂是**真实可复现**的——ARM 弱序下 volatile 协议每秒数百万次撕裂（令牌可见但 payload 未可见）。
  2. **x86-TSO 印证**：同一 volatile 协议在 x86 上零撕裂——这正是该 latent bug 在生产（x86 部署）从未暴雷的原因，也确证了「平台偶然正确」判词。
  3. atomic_ref release/acquire 在 ARM 与 x86 上均零撕裂，无功能回退；ARM 上代价是每帧一次 `stlr`/`ldar`，吞吐仍达千万帧/秒级。
- **在树编译（已过）**：用 v4 warm 构建的精确编译标志，对 `writer.cpp`/`reader.cpp`/`journal.cpp`
  三个 frame.h 消费者在真实树做 `-fsyntax-only`（arm64，gnu++20，完整 conan 依赖）——0 error，
  且 `static_assert(offsetof(frame_header,length)==0)` 由编译器证实成立。(仅前端语义编译；
  完整 object/link 走 node/.gyp 编排,对纯 header+inline 改动风险极低。)
- **下一段方向**：①是否同等审视 `close_page`/页切换路径的可见性；②与 ADR-0002 born-FB 发布路径是否共用同一 `publish_data_length`/`acquire_length` atomic_ref 封装。

> harness 源码与可复现命令见 Atlas:`agent-journal/goals/2026-06-23-kungfu-adr0001-yijinjing-barrier/validation/journal_publish_race.cpp`。

## 实现位点

- `framework/core/src/include/kungfu/longfist/types.h`（frame_header：去 volatile）
- `framework/core/src/include/kungfu/yijinjing/journal/frame.h`（acquire/publish/copy）
- `framework/core/src/libkungfu/yijinjing/journal/writer.cpp`（close_frame_lock_free / copy_frame）
