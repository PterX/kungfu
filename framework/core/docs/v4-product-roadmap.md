# Kungfu 4.0 产品长期目标与分阶段实施计划

> **本文是 v4 产品层的权威恢复点**：任何会话中断/重启,读本文 + `conan2-migration.md` + 记忆
> `kungfu-4.0-kickoff.md` 即可无缝衔接。理解可能片面,以代码为准;重大疑问回原始仓核对。
> 立档：2026-06-17(可人确认 objective 措辞后)。工作仓＝kungfu-trader `dev/v4/v4.0`,经 NAS bare 仓同步双平台。

## 1. 长期目标（Objective，可人 2026-06-17 定稿）

> 把 kungfu 的 journal-first 框架做成一个**非量化的通用「流式数据流运行时 + 平台」**：以 journal/longfist
> 为统一数据平面,提供跨平台多形态客户端(GUI+TUI)的检视/可视化/管理、operator 流式计算与确定性高精度重放,
> 以及**沿 v3 AWS 遗产继续的在线化服务能力(流数据订阅 + 用户身份/配置/在线服务状态等云控制面)**；
> 可人的桌面个人工具是它的第一个持续打磨的用户。

**内核约束(不可动摇)**：
- **journal-first 统一数据平面**：所有形态都消费同一份「带 source/dest/纳秒时间戳/msg_type 的事件 frame +
  longfist 类型」,不另发明数据格式(✅ 已由 Step1-4 双平台点亮)。
- **live/backtest 同源**：确定性重放/仿真复用同一 runtime + 同一 journal/longfist 语义,而非另写引擎。
- **设计哲学：「人类不可靠」→ 机制优先**(继承自 v3)。易错环节收口成固定入口/状态/校验/回放语义/action-feedback 闭环。

## 2. 四根能力支柱

1. **数据平面**：journal/longfist 统一事件模型 + 多语言零拷贝共享运行时（✅ 已点亮，技术北极星）。
2. **客户端层**：GUI(检视/可视化/管理) + TUI(终端速查),共享核心。**Journal Inspector 是第一个交付**。
3. **计算层**：operator 流式计算 / 确定性高精度重放(generalized backtest 的去交易语义形态)。
4. **服务层 / 在线化控制面**：
   - (a) **流数据服务**：journal 远程订阅(websocket frame) + 历史 slice 回放。
   - (b) **云在线化**：用户身份/配置、在线服务状态、云端计算作业(确定性重放/operator at scale)、按需用量/计费；
     **沿 v3 AWS 路线继续,继承 v3 资产作参考(非严格复用旧 kfx)**(清单见 §5)。

## 3. 技术选型（可人 2026-06-17 定：从能力上限选，不为招聘妥协）

- **GUI 外壳 = Electron**(`kungfu_node` 是 N-API addon,需 Node/electron runtime 宿主；亦是 conan 迁移保留 electron 的原因)。
- **GUI 前端 = React + TypeScript**(非 v3 的 Vue3；v3 选 Vue 是为国内招聘,v4 全 AI 开发无此约束)。数据密集 UI
  用虚拟化(TanStack Virtual/Table)、hex 视图等成熟方案。
- **TUI = Ink**(React-for-terminal)。与 GUI 同 React 心智,GUI/TUI 共享组件思维。
- **共享核心 = 一个 TS 库 over `kungfu_node`**(journal 解码/查询/过滤/统计),GUI(Electron+React) 与 TUI(Ink) 都消费它。
- 其余(构建工具/状态管理/组件库)接到各阶段时按最佳实践定,记录于对应阶段文档。

## 4. 分阶段实施计划（Roadmap）

```
P0 现代化地基 ← 进行中(当前)：conan2 全量迁移,让 GUI/TUI/electron/kfc 正式可构建
P1 Journal Inspector MVP(第一个可用版本)：
   ├ 共享核心：TS 库封装 kungfu_node(journal 解码/查询/过滤/统计)
   ├ TUI(Ink) + GUI(Electron+React) 并行,共享核心
   ├ 静态打开检视 + 过滤 + hex + longfist 字段详情 + 导出(json/csv/pcap-like)
   └ location/page 文件管理(列出/大小/帧数/时间；归档/清理走 dry-run+确认)
P2 live tail(边写边看正在写入的 journal) + 回放/注入(replay_writer / 构造帧写入)
P3 operator 流式计算 + 确定性高精度重放(generalized backtest 最小形态)
P4 流数据服务(websocket 远程订阅 + 历史 slice 回放)
P5 云在线化(用户身份/配置、在线服务状态、云端作业；沿 v3 AWS 路线继承为参考)
   └ 贯穿：用户配置 / 在线服务状态 等基础能力可在更早阶段引入轻量版
```

**阶段间约束**：每阶段双平台(Mac arm64 + Linux x64)验证 + 经 NAS 同步 + 建恢复点(记忆/git/文档);
长任务走 tmux;落盘修改先 diff;高风险(真实系统/AWS/删写 journal)先 dry-run。

## 5. v3 可继承资产清单（参考，非严格复用）

详见 atlas `life-evolution/periods/2015-2026-kungfu/kungfu-history-retrospective/`(10-technical-architecture / 20-product-governance)。

**v3 已达到的程度(认知刷新)**：v3 不是「交易终端+Electron 前端」,而是 **journal-first 的完整平台**——
七层(应用入口/产品运行/交易语义/事件运行/数据模型/语言运行时/存储通信),所有形态回到同一 journal/longfist。

**可继承机制**(去交易语义后)：
- **数据层**：`yijinjing/journal/tracer.h`(begin/end 时间窗 + in/out 方向 + 因果追踪 via `trigger_frame_uid`)、
  `journal/assemble.h`(多 locator 合并读 sink/copy_sink)、`replay_writer`(按原始事件重放)、`data`+`CsvSink`
  (CSV↔journal 经 longfist 双向转换)、session index + SQLite cache(`cache/backend.h` boost::hana+sqlite_orm
  从 longfist 自动生成 schema)。
- **计算层**：`Context` 能力面(strategy/operator)、`StreamDataBatcher`(Live/BackTest 两实现共享 batch API)、
  operator first-class(因子/订单簿/合成数据)、`BacktestContext` 的 slice 资源生命周期(SliceIndexer/subscribe_slice)。
- **服务/远程**：`nanomsg/webserver.h`(journal frame 经 websocket 远程订阅;`web_agent`/`websocket_client_server`/`http_server`)。
- **GUI(参考)**：Electron+Vue3,经 `kungfu_node`/watcher + IPC + pm2 接入内核;`framework/app` 已有
  `pages/journal` + `pages/replay`(session 列表/tag/事件面板/时间 slider/过滤/可视化/replay)。
- **TUI/CLI(参考,证据薄)**：`framework/cli`(add/config/list/monit/remove/shutdown/update/export + 终端 UI 组件);
  Python CLI `kfc` 经嵌入 libnode 启 JS CLI。**注：v3 TUI 证据最薄,v4 TUI 需重新设计而非复刻**。
- **AWS 在线体系(v4 沿此路线继续)**：
  - 云服务：`serverless-kungfu`(license/dataset/billing-Lago)、`serverless-kungfu-backtest-runner`(AWS Batch Fargate 云回测容器)。
  - 基础设施：Cognito OIDC 身份 / S3 / SSM / API Gateway / ECR / CloudWatch。
  - 身份计费链：Authing → Lago customer/wallet → Batch 用量。
  - 连接 UI/kfx：`login-authing`(登录/凭证/试用)、`wallet`(余额/充值/用量/账单)、`extension-manager`(插件商店)、
    高精回测 UI、factor UI、feature store(`serverless/store.py`)。
  - **v4 必然需要**：用户配置、在线服务状态、用户身份等——可沿此继承为参考。
  - atlas 侧已有 AWS 中国 + Tokyo relay 基础设施(`infra/aws/`、large-file-overseas-relay skill),v4 在线化可对接。

**引用注意**(回顾文档已标注)：v3 operator/factor/serverless 多由团队实现(Keren author 为 0,集中在跨平台构建/发布/边界);
Web/小程序当时多为「入口壳」未完整承载核心;多个云仓为 shallow 快照未运行验证。引用为「方向参考」,不当「现成能力」。

## 6. 当前状态（恢复指针）

- **数据平面(支柱1)**：✅ Step1-4 双平台点亮(Mac arm64 + Linux x64)。详见 `kungfu-4.0-kickoff.md` 记忆。
- **P0 现代化地基**：✅ **核心达成**(2026-06-18)。conan2 全量迁移完成，详见 `conan2-migration.md`。
  Stage A(conan2 核心)/B(electron)/C(freeze+kfc)/D(Windows) 全通：electron/node/pykungfu/kfc 在 **Mac arm64 / Linux x64 /
  Windows x64** 三平台全可构建，**kfc 三平台 Nuitka freeze 独立运行**(`--version`/`--help` 干净)。`.v4/` bootstrap 已删除退役，
  文档迁至 `framework/core/docs/`。
  **P0 剩余(收尾/优化，非阻塞)**：Stage C ⑤(engage 冻结版打包)/⑦(体积优化)；Windows port(cppstd17/libkungfu STATIC/electron
  disturl/test-torque 补解压)固化进 run-conan.js 的 Windows 路径(目前手动)。
- **P1 产品层**：⬜ 待启动＝**Journal Inspector MVP**(TS 核心 over kungfu_node + Ink TUI + Electron/React GUI；静态检视/过滤/hex/
  longfist 字段/导出 + location 管理)。P0 核心已达成，可进 P1。
</content>
