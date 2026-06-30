# ADR-0008: longfist 二进制布局是跨时间/跨语言兼容的真不变量；schema 演化与 minor 维护策略

- 状态: proposed（确立策略；治理实现待建。longfist=FlatBuffers 与零拷贝 journal 已实现于本分支）
- 日期: 2026-06-29
- 类别: (b) 机制 / 治理 —— 数据格式版本治理
- 子系统: longfist 类型系统 + yijinjing journal（C++/Python/Node 进程内零拷贝 + mmap 跨进程 + 落盘可重放）
- 相关: 与版本机制 `docs/version-release-design.md` 配对（本 ADR 是其「真不变量」那一层）；承接 ADR-0002（longfist hana→FlatBuffers，一次性迁移）转入常态的「演化策略」治理；与 ADR-0001（journal 发布屏障）独立

## 决策

把 **longfist 二进制布局**确立为 kungfu 兼容性的**真不变量**，并据此定 schema 演化与 minor 维护策略：

1. **真不变量在 tag 之下。** git tag、浮动 channel ref、`package.json` version 都是「该 artifact 说哪一版 longfist 布局」的下游投影。真正承重、真正会让消费方「读不出来」的，是 longfist 布局本身。版本机制存在的目的就是保护这个不变量。

2. **布局即 ABI。** 零拷贝意味着内存排布 == journal/wire 排布、读取不经解析。因此一个针对某布局编译的消费方，**无法靠比版本号或特性检测去读另一个布局** —— 只能说同一布局，或在**非零拷贝路径**上做演化感知的显式解码。

3. **热路径 per-minor 钉死、冷路径容演化（杂交）。**
   - **热路径**（进程内零拷贝、实时）：布局在一个 minor 内冻结，跨 minor 不做运行时翻译。这逼出「每个 minor 是一个布局纪元、并行维护」的形态（见 `docs/version-release-design.md` 的「Minor lines are long-lived」节）。
   - **冷路径**（journal 落盘 replay / load）：必须保证「多年前写的 journal 今天可重放」。这里用 **FlatBuffers 的加性 schema 演化**（只增字段、optional、default、不重编号、不改既有字段语义）让**新 reader 解码老布局**，把热路径并行维护的成本 bound 在可接受范围。

4. **兼容承诺按 minor 计、用「runtime ≥ schema 才加载」表达。** 一个针对最低 longfist schema 版本构建的扩展/消费方，应能在 ≥ 该版本的 runtime 上加载；破坏性布局变更只发生在 major、且显式公告。承诺的成熟度必须如实标注，不假装做不到的稳定。

## 背景与问题

kungfu 分发预编译二进制 + 一个被外部产品消费的 capability SDK（ADR-0006）。它的兼容问题与 Python C 扩展 / Node 原生 addon 同类（在编译/ABI 时被消费，运行时特性检测物理上不适用），但**多一层 Python/Node 都没有的维度**：longfist 既跨语言零拷贝、又**持久化进 journal 被 replay**，所以兼容性带一个**时间维**（老数据必须可读）。

历史上文档只治理了 release-line（channel / tag / version-state，见 `docs/version-release-design.md`），把 longfist 当成「schema/磁盘格式」一笔带过（ADR-0001 头注「不动 schema/磁盘格式」即此）。但没有任何文档把 longfist 布局**命名为真不变量**，也没有成文的 schema 演化与 minor 兼容 policy —— **工具（FlatBuffers）选对了，治理 policy 是空的**。本 ADR 补这条。

## 对标

「编译型扩展生态 + 持久化数据格式」的兼容治理，工业界的收敛答案是「稳定子集 + runtime ≥ API」：

- **Python**：完整 ABI 按 minor 锁（`cp3x`），另有 Stable ABI / `abi3`（针对最低版本编、在更新 runtime 上加载）。
- **Node**：Node-API 作稳定层把 addon 与 V8 ABI 抖动解耦，`NAPI_VERSION` 即 runtime ≥ API。
- **数据格式侧**：FlatBuffers / Protobuf / Avro 的加性 schema 演化；SQLite / Postgres 的 on-disk 格式长期兼容（解决「多年前写的数据仍可读」）。

longfist 的题是上述两类**叠加且更硬**（函数 ABI + 跨语言零拷贝数据布局 + 持久化时间维），因此 policy 取它们的交集：稳定子集（FlatBuffers 加性演化）+ runtime ≥ schema + 冷路径跨版本解码。

## 现状（已实现 vs 待建）

- **已实现**（本分支事实）：longfist 用 FlatBuffers（`framework/core` 的 `*.fbs` + `flatc` codegen）；进程内零拷贝 + mmap 跨进程 + journal 落盘；ADR-0002 的 hana→FB 迁移。
- **待建**（本 ADR 确立、实现挂起）：成文的 schema 演化规则校验（CI 挡「改既有字段 / 重编号」的破坏性变更）；「runtime ≥ schema」的加载闸；minor 级兼容窗口声明；冷路径 replay 的跨版本解码测试基线。

## 违背 / 替换判据

任何对 longfist 布局或其分发方式的改动，若**保不住下面任一条即为降级**，须在本 ADR 留痕：

1. 老 journal 仍可被新 binary 在冷路径重放（或提供显式迁移），不得静默打破历史可重放；
2. 一个 minor 内 longfist 布局冻结，破坏性变更只发生在 major 且显式公告；
3. 热路径不因兼容性引入运行时翻译（否则零拷贝 moat 失效）；
4. 每个仍有用户的 minor 线保持「可被任何人独立维护」（不 foreclose；见 `docs/version-release-design.md` 的 maintainability-openness 五条）。
