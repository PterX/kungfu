# Kungfu 4.0 开发台账（最小点亮 → 个人工具）

本目录是 Kungfu 4.0 的开发脚手架与台账。4.0 不再做量化交易，而是在 Kungfu 的技术资产
（C++/Python/Node.js 无缝联动：longfist 跨语言类型 + yijinjing 共享内存 journal + libnode 内嵌运行时）
上开新产品线，先做成「可人自己桌面上用的个人工具」，再按需增量加功能。

- 分支：`dev/v4/v4.0`（从 `dev/v3/v3.2` 拉出，三级机制 dev→alpha→release）
- 目标平台：**仅 Apple Silicon（arm64）原生**，弃 x64
- 工具链：**最新稳定 LTS** —— Python 3.13、Node 22 LTS、conan 2、clang(C++20)

## ⭐ 常驻规则：每个阶段完成后自动建立恢复点

> 每完成一个阶段性工作（一个 Step / 一个里程碑），**必须自动建立恢复点**，无需可人提醒。

恢复点 = 两件事：

1. **更新持久记忆**：`~/.claude/projects/-Users-dkr-Code-atlas/memory/kungfu-4.0-kickoff.md`
   写清当前进度、重建命令、关键移植改动、下一步。
2. **git 检查点提交**：在 `dev/v4/v4.0` 上做一个 WIP 检查点提交（**先展示 git diff**、**不推送**）。
   - 身份：`Claude (Code) <dongkeren+claude@kungfu.link>`，正文加 `Agent: Claude (Code)`，逐次环境变量、不改 git config。
   - 提交信息走轻量 Conventional Commits（`type(scope): 摘要`，不加句号）。

目的：进程被杀 / 冷启动新会话时，磁盘 + git + 记忆三处都能续上，最小摩擦恢复。

## 工具链地基（已装，2026-06-16）

本机为 **M1 Ultra**，但 shell 跑在 **Rosetta（uname=x86_64）**，所有架构敏感命令必须加 `arch -arm64`。

- `arch -arm64 brew install python@3.13 node@22`（arm64）
- conan 2：`PIP_INDEX_URL=https://mirrors.aliyun.com/pypi/simple/ pipx install --python /opt/homebrew/opt/python@3.13/bin/python3.13 conan`
  （pip 默认走局域网 devpi `192.168.100.222:3141`，没有 conan，必须用阿里云镜像覆盖）
- 注意全局 Node 24（`~/.local/opt/node-current`）会遮挡 node@22，需显式用 `/opt/homebrew/opt/node@22/bin`。

## 构建（Step 1：C++ 内核 libkungfu）

本目录 `.v4/` 是一套干净的 conan2 + 直接 cmake 驱动，只编 `libkungfu`（longfist + yijinjing），
不动旧的 conan-1.x 全量编排器 `../conanfile.py`（绑定/打包阶段再正式迁移）。

```bash
cd framework/core/.v4
arch -arm64 conan install . --output-folder=build -s build_type=Release --build=missing
arch -arm64 cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
arch -arm64 cmake --build build -j8
# 产物：build/Release/libkungfu.dylib（arm64）
```

## 构建（Step 2：Python 绑定 pykungfu）

精简 pybind11 模块 `pykungfu`，只绑 longfist + yijinjing（复用 `../src/bindings/python/binding/py-longfist*.cpp` + `py-yijinjing.cpp`），
跳过 libnode(Step3)/wingchun。pybind11 走 conan2 的 `pybind11/2.13.6`（Py3.13 需 ≥2.11），不用 vendored 旧版。

```bash
cd framework/core/.v4
arch -arm64 conan install . --output-folder=build -s build_type=Release --build=missing
arch -arm64 cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DKFV4_BUILD_PYTHON=ON
arch -arm64 cmake --build build -j8 --target pykungfu
# 产物：build/Release/pykungfu.cpython-313-darwin.so
arch -arm64 /opt/homebrew/opt/python@3.13/bin/python3.13 python/test_journal_roundtrip.py
```

> 坑：`publisher` 间接继承 `resource`，纯虚共 4 个；共享 trampoline `PyPublisher` 只覆盖 notify/publish，
> Python 子类化抽象基类会报 `No constructor defined`。解法＝在 `python/pykungfu_lightup.cpp` 用 C++ 定义具体
> `noop_publisher`（实现全部纯虚）并 `py::init<>()` 绑到 yijinjing 子模块，不改共享 trampoline。
> （`writer::close_frame` 末尾无条件 `publisher_->notify()`，单进程写读 demo 无 master 故需 noop。）

## 构建（Step 3：Node 绑定 kungfu_node）

精简 **标准 N-API addon** `kungfu_node`，只绑 longfist + journal 读取（复用 `../src/bindings/node/binding/`
的 `longfist.cpp`/`journal.cpp`/`io.cpp`/`operators.cpp`），入口 `node/kungfu_node_lightup.cpp`。
**关键认知：标准 N-API addon 不依赖 libnode**，对系统 Node 22 头 + node-addon-api 编即可；libnode（进程内嵌
Node 的 shared lib）是 Step 4 才需要的东西。N-API 符号加载时由宿主 node 解析，macOS 用 `-undefined dynamic_lookup`。

```bash
cd framework/core/.v4/node && arch -arm64 npm install   # 取 node-addon-api 头(header-only)
cd .. && arch -arm64 cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DKFV4_BUILD_NODE=ON
arch -arm64 cmake --build build -j8 --target kungfu_node
# 产物：build/Release/kungfu_node.node
# 跨语言点亮：Python 写一帧 Quote → Node 读回同一帧
bash node/test_cross_lang.sh
```

> 坑：现有 Node 绑定的 `Reader` 构造写死 `IODevice::GetDefaultRuntimeLocator()`，即只认环境变量
> **`KF_RUNTIME_DIR`**（默认 `~/.../runtime`），**不认** `openReader()` 所属 io_device 的 base。
> 故读自定义目录的 journal 时，须设 `KF_RUNTIME_DIR` 指向写入目录（编排器已处理）。
> 另：`NODE_API_MODULE(mod, regfunc)` 对 regfunc 做 token 粘贴，须传未限定标识符。

## 进度

- [x] **Step 1**：arm64 编出 C++ 内核 `libkungfu.dylib`。fmt 8.1.1→10.2.1 + spdlog→1.14.1（clang21 拒编 fmt8 的 consteval）；
      源码移植：`common.h` 给 `kungfu::array` 加 `fmt::ostream_formatter` formatter + `fmt/std.h`；`enums.h` 加 ADL `format_as`。
- [x] **Step 2**：pybind11 绑定 `pykungfu`（Python 3.13），conan2 装 `pybind11/2.13.6`。里程碑达成：
      Python 经 journal 写一帧 Quote 再读回，字段一致（`python/test_journal_roundtrip.py`）。
- [x] **Step 3**：标准 N-API addon `kungfu_node`（系统 Node 22 + node-addon-api 8，**不依赖 libnode**）。里程碑达成：
      Python 写、Node 读同一条 journal，`genTime` 完全一致（`node/test_cross_lang.sh`）。三语言（C++/Py/Node）共享 journal 打通。
- [ ] **Step 4**：arm64 从源码重建 libnode（16→22 LTS）→ Python/C++ 经 `node::Start()` 进程内嵌 Node，单进程三语言联动点亮。

## 待办（现代化欠债）

- 完整迁移 `../conanfile.py`（conan 1.x 全量编排器）到 conan 2，用于绑定/打包/freeze。
- fmt/spdlog 升级后复核 `FMT_USE_CONSTEVAL=0` 是否还需要（目前 .v4/CMakeLists.txt 仍留着，可在 fmt10 下去掉验证）。
