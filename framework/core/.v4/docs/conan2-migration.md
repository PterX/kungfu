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

## 4b. 欠债② libnode→dist 的真实形态（2026-06-17 摸清，比预想复杂）

`src/bindings/python/CMakeLists.txt` 与 node 绑定经 `use_libnode()`(`.cmake/libnode.cmake`) 解析
`require('@kungfu-trader/libnode').include/.libpath` → 即从 **npm 包** `@kungfu-trader/libnode` 的
`dist/node/{include, libnode.*}` 取头与库。因此「Stage A 完整 bindings build」**依赖②先完成**。

② 不是单纯拷贝，而是一条现代化链：
1. **libnode 独立仓**(`/Users/dkr/Code/kungfu-trader/libnode`，现 `dev/v16.x`/version 16.15.0)：
   - `dist/` **未被 git 跟踪**(纯本地构建产物，0 tracked)，`.git` 1.1G(含 node 子模块历史)。
   - dist 装配逻辑＝`.gyp/node-dist.js`：拷 `libnode.*` + 三处 headers(`node/src`、`node/deps/v8/include`、
     `node/deps/uv/include`，flatten 进 `dist/node/include/`) + 建 `libnode.dylib`/`.so` 软链。
   - 现 `dist/node/libnode.83.dylib` 是 **stale x86_64 / Node16.15**(2021)，要换 Node22。
   - 我们的 Node22 源码树：Mac=`.node22-scratch/node-v22.22.3`、Linux=`~/Code/kungfu-worktrees/node22-src/node-v22.22.3`。
2. **跨机(plan a)**：两机各自 libnode 仓铺各自平台 dist(Mac .dylib / Linux .so.127)。Ubuntu 现无 libnode 仓 →
   需同步(建议给 libnode 仓也建 NAS bare 仓，类比 kungfu；但 dist 是本地 untracked，只需同步源码部分)。
3. **kungfu-core 消费端**：`package.json` devDep `@kungfu-trader/libnode: 16.15.0`→22.x、`electron: 19.1.8`→现代版；
   node_modules 未装；dev 阶段(D5)用 **local link/file 依赖**指向本机 libnode 仓(Node22 dist)，prebuilt 分发留后。

**决策(可人 2026-06-17)**：覆盖旧 dist；plan a(两机各自铺)。**待定**：electron 目标版本(见 §5⑤)；libnode 仓是否建 NAS bare 仓。

## 5. 待验证假设（动手前/动手中需确认，勿当既定事实）

1. **一份 N-API 二进制能否同时供 node 与现代 electron**？N-API ABI 稳定，理论上 `NAPI_VERSION=8` 的
   kungfu_node.node 对二者通用（可省去 electron-runtime 单独构建）。**待用 electron 实测**；若不通用则保留双产出。
2. **Nuitka 2026 能否快速完整生成可执行 kfc**？需实测；决定 D5 默认 freezer。
3. **poetry/uv 能否冻出独立、不依赖用户 Python 的 kfc**？能则迁，否则保留 pipenv（§2.6）。
4. pybind11：.v4 用 conan `pybind11/2.13.6`；主构建历史用 vendored `.deps/pybind11`(旧 2.9.0，Py3.13 不够)。
   迁移后统一用 conan pybind11（A-2a 已把主 CMakeLists 的 `add_subdirectory(.deps/pybind11-2.9.0)` 换成
   `find_package(pybind11)`；待 bindings 子目录实编验证 pybind11 target 名是否匹配）。
5. **electron 目标版本**：现 kungfu-core devDep `electron: 19.1.8`(EOL)。Node22 现代化需配套现代 electron
   （且 N-API ABI 跨 electron 版本稳定）。目标版本待可人定。

## 6. 变更记录（逐次理解/决策追加于此）

- 2026-06-17：建立本文档。完成对构建编排全景的通读理解（conanfile.py / 主 CMakeLists / .gyp/run-conan.js /
  run-build.js / package.json / libnode.cmake），可人确认 §2 历史成因，定 §3 决策、§4 分阶段规格。下一步＝Stage A。
- 2026-06-17 Stage A-1：`conanfile.py` 由 conan1 端口到 conan2。关键 API 映射：`from conans`→`from conan`；
  `generators="cmake"`+`imports()`→`generate()`(CMakeDeps+CMakeToolchain，generator=Ninja，透传 SPDLOG_LOG_LEVEL_COMPILE)；
  `tools.detected_os()`→`_detected_os()`(platform.system 包装)；`self.copy`→`conan.tools.files.copy(self,…)`；
  `tools.Git`→subprocess git；`tools.mkdir/chdir/which`→`os.makedirs`/`os.chdir`/`shutil.which`；options 用 `pkg/*:opt`
  形式；deps 升 fmt10.2.1+spdlog1.14.1。`imports()` 删除——其「拷 Python headers/lib」职责移到 package()(对 kfx
  开发者导出)，内部构建改 CMake FindPython（决策 D6）。验证：`conan inspect` 加载成功(10 requires/options 正确)；
  `conan install -o &:with_yarn=False` 成功，generate() 产出 conan_toolchain.cmake + 各 *Config.cmake(Mac arm64)。
  **待验证**：build()/package()/freeze 的运行逻辑尚未跑通(下一步随主 CMakeLists conan2 适配 + libkungfu 构建验证)；
  configure() 里 libcxx 设定暂留空(交给 conan profile，避免与 LAN 缓存包不一致)，待 Windows 阶段复核 vs_toolset。
- 2026-06-17 Stage A-2a：主 `CMakeLists.txt` 的 conan2 桥接。`include(conanbuildinfo.cmake)+conan_basic_setup(...)`
  →conan2 `find_package(fmt/spdlog/nlohmann_json/nng/rxcpp/SQLite3/RocksDB/tabulate/pybind11)` + `set(CONAN_LIBS …)`
  + `link_libraries(${CONAN_LIBS})`(照 .v4 已验证桥接)，使 `src/libkungfu/CMakeLists.txt` 的
  `target_link_libraries(... ${CONAN_LIBS})` 零改动复用。`add_subdirectory(.deps/pybind11-2.9.0)`→
  `find_package(pybind11)`(conan 2.13.6，旧 2.9.0 不支持 Py3.13)。验证：configure-only 全目标(含 pybind11)解析成功；
  libkungfu-only 实编 `libkungfu.dylib` 链接成功(Mac arm64)。**待验证**：node/python 绑定子目录(use_libnode→需 libnode)
  与 `conan build` 全流程；run-conan.js 的 conan1 flags(-if/-bf/-pf)→conan2(--output-folder/CMake helper 传 toolchain)。
- 2026-06-17 摸清欠债②真实形态(见 §4b)：发现 Stage A 完整 bindings build 依赖②(libnode→dist 经 use_libnode)，
  且②是一条现代化链(libnode 仓 dist+版本升+跨机同步 + kungfu-core devDep electron/libnode 升级 + npm link)，非单纯拷贝。
  可人定：覆盖旧 dist、plan a。下一步排序：先做② → 再回 A-2b 完整 bindings → B/C/D。
- 2026-06-17 ②双平台 dist 落位完成：可人定 electron→agent 选 **Electron 37.x**(捆绑 Node22+V8 13.7，N-API/ABI 与
  libnode 22.22.3/ABI127 对齐；具体补丁号接 devDep 时核实)；libnode 仓建 NAS bare 仓(同意)。
  执行：①NAS `…/Drive/git/libnode.git` 建好，Mac libnode 接 nas remote 推 dev/v16.x + 新建 **dev/v22.x**(version
  16.15.0→22.22.3、node-addon-api ^5→^8，commit `f5d495b5`，仅 NAS 不推 github；node 子模块 retarget 暂缓——dist 已
  Node22，v4 不经子模块重编)。②Mac dist/node 铺 Node22 arm64(libnode.127.dylib+333头+软链)，npm shell 解析✓。
  ③Ubuntu 从 NAS clone libnode(dev/v22.x，无子模块)，dist 铺 Node22 x64(libnode.so.127+333头+软链)，解析✓。
  **dist 铺法**(两机通用，照 node-dist.js)：`rm -rf dist/node; cp out/Release/libnode.{127.dylib|so.127} dist/node/;
  rsync -am --include='*/' --include='*.h' --exclude='*' {src/,deps/v8/include/,deps/uv/include/} dist/node/include/;
  ln -sfn libnode.* dist/node/libnode.{dylib|so}`。**下一步 A-2b**：kungfu-core package.json devDep
  `@kungfu-trader/libnode 16.15.0→22.22.3`(dev 用 local link 指向本机 libnode 仓)+`electron 19.1.8→37.x`，npm/yarn
  install 让 use_libnode 解析→ run-conan.js flags conan2 化 → 完整 bindings build(真 pykungfu 内嵌 libnode + kungfu_node)。
- 2026-06-17 A-2b 进行中(消费端接通 + 真实绑定首编)：
  - kungfu-core `package.json` devDep：`electron 19.1.8→^37.10.3`(最佳实践选 Electron 37 LTS-line，捆绑 Node22；域内镜像
    最新 37.10.3)、`node-addon-api ^5→^8`、删 `nan`(已被 node-addon-api 取代)；**`@kungfu-trader/libnode` 不进 committed
    package.json**(未发布、且两机 dist 路径不同)，dev 走 **npm link**(机器本地不提交)。
  - **dev npm 摩擦(重要)**：`npm install` 会全树 reconcile，撞未发布的 libnode 版本(ETARGET)并清掉 npm link 符号链接。
    故 dev bootstrap＝①`npm install`(registry deps，但 kungfu-core 是 lerna monorepo 含多内部 @kungfu-trader/* 包，全量
    install 本身是更大未决项)②`npm link @kungfu-trader/libnode`(指向本机 libnode 仓)③node-addon-api 铺入 node_modules。
    **当前用最小 node_modules**(cp node-addon-api + link libnode)验证，未做全量 monorepo install。
  - **验证**：完整 configure(libkungfu+kungfu_node+pykungfu 全 enabled，node-addon-api/libnode 头解析✓)；手动 cmake 实编
    出真实 **libkungfu.dylib + pykungfu**(含 py-libnode 内嵌，otool 链 `@rpath/libnode.127.dylib`✓，Mac arm64)。
  - **两个构建驱动待办(非 blocker，是驱动细节)**：①kungfu_node 需 `NODE_RUNTIME=node/electron` 才触发 build_node_binding；
    且 build() 的 pure-cmake node 分支历史上**显式 skip kungfu_node+pykungfu 只编 libkungfu**——真实双绑定走 **cmake-js
    (with_yarn) 路径**(历史成因#2)。②pykungfu 误链 Python 3.14(Mac 默认)，需 `-DPYTHON_EXECUTABLE` 钉到受控 3.13。
  - **下一步**：run-conan.js conan1 flags→conan2 + 跑通 cmake-js(with_yarn) 路径(yarn+cmake-js，需 JS 工具链/monorepo
    bootstrap)出 kungfu_node(node+electron 双 runtime) + pin Python；或先验证 conan build 端到端。Linux 侧同步重做。
- 2026-06-17 A-2b cmake-js+conan2 集成打通(Mac，**最大不确定项已验证**)：
  - cmake-js 独立装(7.4.0，`/tmp/cmjs`，LAN registry `--ignore-scripts`)；从 kungfu-core 跑(node_modules 有 node-addon-api
    + npm-link libnode)。**关键集成法**：`cmake-js configure/build --runtime node --runtime-version 22.22.3 --arch arm64
    --out <dir> --CDCMAKE_TOOLCHAIN_FILE=<conan_toolchain> --CDCMAKE_BUILD_TYPE=Release --CDSPDLOG_LOG_LEVEL_COMPILE=...`
    cmake-js 传 `-DNODE_RUNTIME=node` + 透传 conan toolchain → 二者共存无冲突。设 `NODEJS_ORG_MIRROR=https://cdn.npmmirror.com/binaries/node`
    让 cmake-js 经域内镜像下 runtime headers(node/electron 都快)。
  - **验证(Mac arm64)**：runtime=node 编出真实 **kungfu_node.node**(含 watcher/全 store)+ drone.node，零错误，链 libkungfu✓；
    runtime=electron configure 通过(下 electron 37.10.3 headers、`NODE_RUNTIME=electron`、目标 kungfu_electron 解析、configure
    done)→ **B 阶段已 de-risk**。
  - **A-2b 剩余**：①pykungfu pin Python 3.13(`--CDPYTHON_EXECUTABLE` 或 cmake -DPYTHON_EXECUTABLE，现误抓 Mac 默认 3.14)；
    ②run-conan.js 的 conan1 flags(-if/-bf/-pf + getNodeVersionOptions 读 libnode devDep——已移出 package.json 需改读源)→conan2；
    ③electron full build(kungfu_electron) + ④Linux 侧整条 cmake-js 路径重做。cmake-js 产物在 /tmp(非仓)。
- 2026-06-17 A-2b Mac 侧 ①②③ 全绿：
  - **①Python pin**：cmake-js `--CDPYTHON_EXECUTABLE=/opt/homebrew/opt/python@3.13/bin/python3.13` → pybind11 FindPython
    锁 3.13.13，pykungfu 产物变 **cpython-313**(原误抓 Mac 默认 3.14)。kungfu-core `config.python_version` 3.9→3.13。
  - **②run-conan.js conan2 化**：`-if/-bf/-pf`→`--output-folder build` + `--build=missing`；`makeConanOptions` 去掉
    `arch`(conan2 arch 是 setting 由 profile 自测，非 -o)；`getNodeVersionOptions` 改：electron 从 devDep 去 ^/~、
    node_version 从 `config.node_version`(22.22.3，因 libnode 移出 devDep)。conanPackage 暂用 conan build 占位(freeze
    属 Stage C，conan2 无独立 conan package，需 export-pkg 重构)。conanfile `__build_cmake_js_cmd` 补：conan arch
    (armv8/x86_64)→node arch(arm64/x64) 映射 + **透传 conan_toolchain.cmake(--CDCMAKE_TOOLCHAIN_FILE)+CMAKE_BUILD_TYPE**
    给 cmake-js(orchestrated 路径必需，编码我手动验证过的集成法)。config.arch(硬编 x64)删除。
  - **③electron full build**：cmake-js `--runtime electron --runtime-version 37.10.3` 编出 **kungfu_electron.node**(arm64)，
    零错误 → B 阶段 Mac 完整验证(非仅 configure)。
  - 验证：conanfile py_compile + conan inspect、run-conan.js `node -c` 全过。**剩 ④Linux 侧整条重做**(node+electron+Python pin)。
</content>
