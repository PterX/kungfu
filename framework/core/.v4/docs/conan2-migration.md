# kungfu-core 构建系统 conan2 迁移 — 理解与决策日志

> **读法与边界（重要）**：本文是 agent 对 kungfu-core 构建编排的**理解快照 + 迁移决策逻辑链**，
> 目的是让未来的我/其他 agent 不必重读全部代码即可深入理解项目。但**这份理解可能片面、待补充**；
> 遇到与代码冲突或重大疑问时，**一律以代码为准**，并回头修订本文。历史成因部分由可人（项目原作者）
> 口述确认，已标注。
>
> 范围：`framework/core`（dev/v4/v4.0 分支）。目标＝把 conan-1.x 全量编排迁到 conan2，
> **不要向下兼容 v3，只保证 v4+ 可用**；跨平台 Mac arm64 + Linux x64 + Windows。

## 1. 构建编排全景（现状）

kungfu-core 不是单一 cmake 工程，而是一套多层编排：

```
package.json scripts
  ├─ configure → .gyp/run-conan.js install   ┐
  ├─ compile   → .gyp/run-conan.js build      ├─ pipenv run conan(1.x)：conanfile.py
  ├─ freeze    → .gyp/run-conan.js package    ┘    驱动 cmake / cmake-js / freeze
  └─ install/build/package → .gyp/run-build.js → node-pre-gyp(prebuilt.libkungfu.cc) + wheel
conanfile.py(conan1)
  ├─ requires: fmt/nlohmann_json/nng/rxcpp/sqlite3/spdlog/tabulate/rocksdb/gtest
  ├─ build():   对 node + electron 两个 runtime 各跑一遍构建
  │     __enable_modules: libkungfu 恒建；node-runtime→pykungfu；electron-runtime→kungfu_node
  │     with_yarn=True → cmake-js(取 runtime headers)；否则纯 cmake
  ├─ imports(): 拷 Python include/lib（conan1 专有）
  └─ package(): freeze(kfc) 或 拷 头/库/deps
主 CMakeLists.txt: conan1 conan_basic_setup + .cmake/{editor,compiler,libnode} + vendored .deps/{hana,sqlite_orm}
                   用 KUNGFU_BUILD_SKIP_* 环境变量条件编模块
```

产物三类：**libkungfu**(C++ 内核)、**pykungfu**(Python 扩展)、**kungfu_node**(Node/Electron N-API addon)；
加 **kfc**(把 Python core 冻成独立可执行，PyInstaller/Nuitka)。

## 2. 历史成因（可人 2026-06-17 确认）

1. **双 UI 形态**：要同时支持 **GUI（Electron）** 和 **TUI（原生 node）**两种 UI。因此 kungfu_node
   需对 electron 与 node 两形态产出——**除非现代 electron 与 node 能用同一份二进制**（见 §5 待验证①）。
2. **两条构建路径都保留**：
   - `with_yarn=True`：在**总仓 workspace 下 `yarn build`** 时生效（yarn 传一系列环境变量）。
   - 纯 cmake：**只针对 kungfu-core 开发**时直接 `conan build` 走。
   - 原则：**对依赖尽量自主可控、把开发者的安装需求降到最低**；`cmake-js` 取 runtime headers 正是此原则的体现。
3. **kfc freeze 必须保留**：让用户**无需另装 Python** 即具备完整 Python 能力。当年 Nuitka 还不够成熟
   （不能快速完整生成可执行 kfc），故用 PyInstaller；**若现在 Nuitka 成熟则优先 Nuitka**（见 §5 待验证②）。
4. **`imports()` 拷 Python headers/lib 是交付物**：给 **kfx 插件开发者**用同版本 header 构建——
   需为 kfx 开发者提供 kfc 自身所需的一切依赖。→ 内部构建可改用 CMake `FindPython`，但**打包仍必须导出
   Python headers/libs**（不能只 FindPython 了事）。
5. **prebuilt 分发**：最终要走 `prebuilt.libkungfu.cc` 预编译分发；**开发阶段先以本地源码构建为主**。
6. **pipenv**：当年保留是因 pipenv 对底层包（尤其 Python 二进制、setuptools）封装更彻底，冻出的 kfc 才
   **独立、不依赖用户机器的 Python**。**若 poetry/uv 能等效则完整迁移，否则保留 pipenv**（见 §5 待验证③）。

## 3. 迁移决策（逻辑链）

- **D1 不要 v3 兼容**：可丢弃 v3-only 死代码，不回归测 v3；唯一验收标准＝v4+ 在 Mac/Linux(/Windows) 可用。
- **D2 .v4 折叠退役**：`.v4/`（conanfile.txt + .v4/CMakeLists + KFV4_* 开关）是 Step1-4 的最小点亮 bootstrap，
  已双平台验证 conan2 + fmt10 + 三语言联动可行。迁移即把它的成果**提升为正式 `framework/core/conanfile.py`
  + 主 CMakeLists**，随后 `.v4/` 退役。
- **D3 deps 升级**：fmt 8.1.1→10.2.1、spdlog 1.10.0→1.14.1（已在 .v4 验证；并已去掉 FMT_USE_CONSTEVAL=0）。
- **D4 保留两条构建路径 + cmake-js**：按 §2.2，with_yarn(cmake-js) 与纯 cmake 都留。
- **D5 freezer 默认**：先评估 Nuitka 2026 成熟度；成熟则默认 Nuitka，否则 PyInstaller（§5②）。
- **D6 Python 依赖交付**：内部构建用 FindPython；打包阶段保留导出 Python headers/libs 供 kfx（§2.4）。

## 4. 迁移分阶段 + 逐文件改动规格（dry-run）

**Stage A — conan2 核心（Mac/Linux 验证）**
- `conanfile.py`：`from conans import *`→`from conan import ConanFile` + `conan.tools.{cmake,files,scm}`；
  `generators="cmake"`→`generate()` 用 `CMakeToolchain`+`CMakeDeps`；`imports()` 删除（→ FindPython + 打包期导出）；
  `tools.detected_os()`→`self.settings.os`/`platform`；`self.copy`→`copy(self,…)`；`tools.Git`→`conan.tools.scm.Git`；
  options/default_options 改 conan2 格式（`"fmt/*:header_only"`）；deps 升级(D3)。
- 主 `CMakeLists.txt`：`include(conanbuildinfo.cmake)+conan_basic_setup(...)`→conan2 `find_package(...)`+target 链接
  （照 .v4 已验证写法）；保留 `.cmake/{editor,compiler,libnode}` 与 vendored `.deps`。
- `.gyp/run-conan.js`：conan1 flags `-if/-bf/-pf`→conan2 `--output-folder`/layout；`conan build/package` 适配；options 语法。
- 验收：`conan install`+`conan build` 出 libkungfu + pykungfu + kungfu_node（Mac arm64 + Linux x64）。

**Stage B — electron 运行时**：cmake-js `--runtime electron` 在 conan2 下跑通；验证 §5① 一份二进制能否同时供 node+electron。

**Stage C — freeze + kfc**：PyInstaller/Nuitka(按 D5) + node-pre-gyp 打包在 conan2 下跑通；kfc 独立可执行（不依赖用户 Python）。

**Stage D — Windows**：vs_toolset/ClangCL、`.deps/vs`、kungfu_node.cpp 的 libnode delay-load；agent 做端口 +
给 DARKHERO 的验证脚本，**Windows 绿与否由可人侧实测**（agent 此环境无法充分验 Windows）。

**收尾**：`.v4/` 折叠退役；本文档与记忆更新。

## 5. 待验证假设（动手前/动手中需确认，勿当既定事实）

1. **一份 N-API 二进制能否同时供 node 与现代 electron**？N-API ABI 稳定，理论上 `NAPI_VERSION=8` 的
   kungfu_node.node 对二者通用（可省去 electron-runtime 单独构建）。**待用 electron 实测**；若不通用则保留双产出。
2. **Nuitka 2026 能否快速完整生成可执行 kfc**？需实测；决定 D5 默认 freezer。
3. **poetry/uv 能否冻出独立、不依赖用户 Python 的 kfc**？能则迁，否则保留 pipenv（§2.6）。
4. pybind11：.v4 用 conan `pybind11/2.13.6`；主构建历史用 vendored `.deps/pybind11`(旧 2.9.0，Py3.13 不够)。
   迁移后统一用 conan pybind11（待确认主 CMakeLists/bindings 对 pybind11 的取用方式）。

## 6. 变更记录（逐次理解/决策追加于此）

- 2026-06-17：建立本文档。完成对构建编排全景的通读理解（conanfile.py / 主 CMakeLists / .gyp/run-conan.js /
  run-build.js / package.json / libnode.cmake），可人确认 §2 历史成因，定 §3 决策、§4 分阶段规格。下一步＝Stage A。
</content>
