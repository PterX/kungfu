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
- 2026-06-17 **A-2b 双平台闭环完成** ✅：
  - **④Linux 侧**：Ubuntu 装 python3.12-dev(无 python3.13，Linux pykungfu 用 3.12；Mac 用 3.13，分歧记为后续统一项，kfc
    freeze 本就按平台)、cmake-js 7.4.0(`/tmp/cmjs`)；node_modules＝cp node-addon-api + `npm link @kungfu-trader/libnode`
    (先 `cd ~/Code/libnode && npm link` 注册全局再链接，否则报 module not found)；主 conanfile.py `conan install --output-folder
    build-conan2`(gtest 源码编)。cmake-js 集成同 Mac，但 **`--disturl https://cdn.npmmirror.com/binaries/node`**(cmake-js 不认
    NODEJS_ORG_MIRROR；electron headers 仍走 electronjs.org，~30MB 一次性)。
  - **产物**：Linux node runtime 编出 kungfu_node + drone + **kungfu_kfc**(Linux 专属，链 libnode) + pykungfu.cpython-312；
    electron runtime 编出 kungfu_electron.node(ELF x86-64)。Mac arm64：kungfu_node + kungfu_electron + pykungfu.cpython-313
    + drone。**全部经 conan2 + cmake-js + Node22 libnode 跑通，零源码错误**。
  - **A-2b 完成**。P0 剩：**C(freeze+kfc，先评 Nuitka 2026 成熟度 + conan2 无独立 conan package 需 export-pkg 重构 freeze 入口)**、
    **D(windows，agent 端口 + DARKHERO 实测)**、然后 .v4 退役。
  - **遗留小项**：Mac/Linux pykungfu Python minor 分歧(3.13/3.12，待统一)；kungfu-core 全量 JS monorepo install 仍未做(当前用最小
    node_modules：node-addon-api + link libnode)；orchestrated 端到端(yarn→run-conan→conan build→build()→cmake-js)未跑通(需
    pipenv + monorepo bootstrap)，但底层能力已由直接 cmake-js 验证。

## 4c. Stage C(freeze + kfc)分析（2026-06-17 摸清，是 Python 栈现代化子工程）

kfc＝把 `kungfu` Python 包(`src/python/kungfu`，入口 `__main__.main()`→console select)+ numpy/pandas/plotly 等
冻成**独立可执行**(用户无需装 Python)。两条 freezer 路径都在：PyInstaller(`src/python/kfc.spec`，详细 spec)、
Nuitka(`src/python/kfc.py` 内嵌 `--standalone --include-package=numpy/pandas/plotly --enable-plugin=anti-bloat/numpy`)。
构建哲学：**pipenv 造 venv + 定 Python 版本；poetry 管其余依赖**(`pyproject.toml` packages=kungfu from src/python)。

**前置冲突/现代化点(动手前必解)**：
1. **Python 版本**：`pyproject.toml` 钉 `python = ">=3.9, <3.12"`，与 v4 runtime(Mac pykungfu 3.13 / Linux 3.12)冲突。
   **且 kfc 冻的 Python 必须＝pykungfu 的构建 Python**(kfc 进程 import pykungfu)。故 freeze Python 要么统一(两平台都
   3.13，需 Ubuntu 装 python3.13)，要么按平台冻(Mac3.13/Linux3.12)。
2. **数据栈版本旧**：numpy ~1.25(不支持 3.13)、pandas ~2.0、scipy ~1.12、nuitka ~1.5.0(2026 是 2.x)、pyinstaller ~5.13.2
   ——配 Python 3.13 需整体升级(numpy≥1.26/2.x 等)。
3. **freezer 选择**：可人倾向 Nuitka(若 2026 成熟)。Nuitka 2.x 在 2026 已成熟(standalone + numpy/pandas plugin 可用)，
   产出真编译二进制(更独立、更难逆向)；PyInstaller 5→6 更快但是 bundle 非编译。
4. **conan2 freeze 入口**：conan2 无独立 `conan package`，freeze 需脱离 conanfile.package()(直接驱动 Nuitka/PyInstaller，
   或 conan export-pkg)。dist/kfc 产物路径。
5. **env bootstrap**：pipenv(造 venv+Python)+ poetry(装数据栈)+ pykungfu 可导入。本机有 pipenv/poetry(`~/.local/bin`)，无 venv。

**决策(可人 2026-06-17)**：①统一 Python **3.13** 双平台(Ubuntu 也装 python3.13) ②freezer **Nuitka 2.x**(不行回退
PyInstaller) ③数据栈升到 **3.13 兼容最新稳定**。

**Stage C 基础已落(2026-06-17)**：依赖现代化(决策驱动、可提交)——
- `pyproject.toml`：python `>=3.9,<3.12`→`>=3.13,<3.14`；numpy `~1.25`→`~2.1`、pandas `~2.0`→`~2.2`、scipy `~1.12`→`~1.14`、
  statsmodels→`~0.14.4`、nuitka `~1.5`→`~2.5`、pyinstaller `~5.13`→`~6.11`(dev fallback)、black/pdm/scons/click/psutil/
  tabulate/wcmatch/urllib3/websockets/boto3/keyring/zstandard/setuptools/virtualenv/wheel 等同步升 3.13 兼容；**移除 vestigial
  conan ~1.60 dev-dep**(conan2 由系统 pipx 提供)；dev-deps 段 `[tool.poetry.dev-dependencies]`→`[tool.poetry.group.dev.dependencies]`。
- `Pipfile`：pip `23.2.1`→`24.3.1`、setuptools→`75.6.0`、virtualenv→`20.28.0`、wheel→`0.45.1`、poetry `1.6.1`→`1.8.5`、
  urllib3→`2.2.3`、xattr→`1.1.4`、cffi→`1.17.1`、pycparser→`2.22`、cryptography→`43.0.3`、pywin32-ctypes→`0.2.3`；加 `[requires] python_version="3.13"`。

**Stage C env bootstrap + kfc 运行验证完成(2026-06-17，Mac)**：
- **pipenv 升级**：旧 pipenv 2022.8.15(自身在 py3.7)在 3.12+ venv 缺 `pkg_resources`(setuptools 不再自带)崩溃 →
  `pipx uninstall pipenv && pipx install pipenv --python <py3.13>` 装 **2026.6.2**。
- **bootstrap**：`pipenv --python /opt/homebrew/opt/python@3.13/bin/python3.13 install`(造 3.13 venv `core-wguTdosw` +
  poetry 1.8.5)→ `pipenv run poetry lock`(**3 轮修冲突**：railroad-diagrams `~3.1`不存在→`*`、certifi `~2023`被 pdm 要求
  `≥2024.8.30`→`*`、一批传递/工具类 pin 放宽 `*`)→ `pipenv run poetry install`(65 装)。pyproject 加 `[[tool.poetry.source]]`
  aliyun 国内源(避免占出海链路)。**验证**：numpy 2.1.3 / pandas 2.2.3 / scipy 1.14.1 / nuitka 2.5.9 全在 py3.13。
- **kfc 运行验证**：`PYTHONPATH=<pykungfu Release dir> pipenv run python src/python/kfc.py --help` 输出完整 kungfu CLI
  (assemble/login/run/backtest/journal/slicetool/tool/engage)。full pykungfu(longfist/yijinjing/wingchun/libnode)import 成功。
  **改 1 处源码**：`kungfu/console/commands/__init__.py` 的 `from click.decorators import F as CLI` → 本地 TypeVar(click 8.1.7+
  移除私有 F)。**坑**：`kungfu/__init__` 读 pykungfu 同目录的 `kungfubuildinfo.json`(version 字段)；conanfile.build() 的
  `__gen_build_info` 才生成它，**cmake-js 直编路径不生成**——freeze/运行前需补生成(本轮手造最小 json 验证)。

**Stage C Nuitka freeze 进行中(2026-06-17 Mac，已出 bundle，卡在 stdlib/hook 迭代)**：
- **freeze 命令**：`cd framework/core && PYTHONPATH=<pykungfu Release dir> pipenv run python -m nuitka --output-dir=/tmp/kfc-nuitka
  --assume-yes-for-downloads --include-package-data=certifi src/python/kfc.py`(kfc.py 内嵌 --standalone + include numpy/pandas/plotly
  + anti-bloat)。**产物**：`/tmp/kfc-nuitka/kfc.dist/`(1.0G)：`kfc.bin`(380M) + `Python` + **pykungfu.so + libkungfu.dylib +
  libnode.127.dylib 全打进来**(✓ native 库打包成功)。
- **已解坑**：①pykungfu 链 `@rpath/libnode.127.dylib` 但该 dylib 不在 pykungfu 同目录 → Nuitka FATAL。修：把
  `libnode.127.dylib` 从 libnode dist 拷到 pykungfu 旁(Release dir)，三 native 库同目录后 Nuitka 正常打包。
  ②`kungfubuildinfo.json`(kungfu/__init__ 读 pykungfu 同目录的它)cmake-js 路径不生成 → 需手造/补生成(freeze 后还要拷进 kfc.dist)。
- **当前卡点(下次起点)**：kfc.bin 干净环境运行，过了解释器启动+kfc.py+kungfu init+命令注册，**卡在 certifi**：
  certifi 2026.05.20 core.py 用 `if sys.version_info>=(3,11)` 选分支，但**冻结二进制走了 else(<3.11)分支**(line 51
  `from importlib.resources import path,read_text`，3.13 已移除这两符号)→ ImportError。同时启动期反复 `import warnings failed:
  No module named 'warnings'`。**两者同源＝Nuitka 的 stdlib 打包/分支处理异常**。**修复假设**(下次试)：①去掉过度精简的
  `--python-flag=no_warnings` 与已废弃的 `--enable-plugin=numpy`(Nuitka 警告其 deprecated)、评估 anti-bloat 是否过度剥 stdlib；
  ②certifi else-branch 误入暗示 Nuitka 版本分支处理 bug，试 `--include-module=importlib.resources` 或针对 certifi 的 nuitka 选项；
  ③这条 certifi/authing/requests 是 login(serverless SSO)拉进来的外围 web 栈，P1 Journal Inspector 不需要——必要时可先用更小入口
  验证 freeze 主路径，web 栈单独修。④certifi cacert.pem 已 --include-package-data=certifi。
- **freeze 入口**：当前直接 `python -m nuitka`(脱离 conan2 package()，符合 D6)。正式化时封装成脚本/run-conan freeze 子命令。

**Stage C Nuitka freeze 调通 ✅(2026-06-17 Mac，kfc.bin 干净独立运行)**：
- **certifi 卡点真因(推翻原假设)**：用最小 probe(standalone 冻结) 实证——**冻结后 `sys.version_info` 正确是 (3,13,13)、`>=3.11` 为
  True、`importlib.resources` 的 files/path/read_text 全在**。certifi 并非「冻结后版本变 <3.11」，而是 **Nuitka 2.5.9 的
  anti-bloat 配置 (`standard.nuitka-package.config.yml` 的 `certifi.core`) 对新版 certifi 失效**：它把源码里 `sys.version_info`
  文本替换成 `(0,)`(→`if (0,)>=(3,11)` 恒 False，强制走 else 分支)、再把 else 分支那行 `from importlib.resources import
  path as get_path, read_text` 替换成 `raise ImportError`。这套替换是为**老 certifi**(else 分支带 try/except 文件兜底)写的；
  certifi 2026.05.20 的 else 分支是裸 import 无兜底 → `raise ImportError` 未被捕获 → 整个 `import certifi` 崩。`import warnings
  failed` 噪声则由 `--python-flag=no_warnings` 引起(probe 去掉即消失)。
- **修复＝升级 Nuitka 2.5.9→4.1.2**：4.1.2 的 `certifi.core` 同样替换但加了 `when: 'version("certifi") < (2025,)'` 门控；
  我们 certifi 2026.x ≥2025 → 替换不再应用，**保留 anti-bloat 的同时干净修好 certifi**(probe 实证 + 全量 kfc 验证)。2.5.9 的
  user-package-config 不允许覆盖已存在 module-name(`update` 断言)，故走升级而非 yaml 覆盖。`pyproject.toml` nuitka pin `~2.5→~4.1`。
- **kfc.py 选项随 4.x 调整**：去 `--python-flag=no_warnings`(清启动噪声)；去 `--enable-plugin=numpy`(4.x 自动处理、且 deprecated)
  与 `--enable-plugin=anti-bloat`(4.x 恒开，显式启用会 WARNING)；`--noinclude-custom-mode=distutils:nofollow`(4.x 废弃 `:mode`
  语法，FATAL)→`--nofollow-import-to=distutils`。
- **mypyc 包次生坑(4.x 比 2.5.9 严)**：2.5.9 能编入随包发 mypyc `.so` 的包，4.1.2 拒绝把包的 `__init__.cpython-313-darwin.so`
  当源码解析(SyntaxError invalid encoding)。命中 ①chardet 7.x(requests 可选探测器，缺失回退 charset_normalizer，已 nofollow)
  ②`kfc engage` 的 dev 工具桥接 black/pdm/SCons/scons/nuitka(全 mypyc，且把 Nuitka/SCons 自身打进冻结 kfc 不合理)。bridging 全是
  函数内懒加载，nofollow 不影响 kfc 启动与核心命令；`kfc engage <tool>` 留作 engage 打包另案(见 Stage C 剩余⑤)。均在 kfc.py 加
  `--nofollow-import-to=`。
- **产物 & 验证**：`/tmp/kfc-nuitka/kfc.dist/`(910M)：kfc.bin(277M)+pykungfu.so+libkungfu.dylib(419M)+libnode.127.dylib(110M)。
  补 `kungfubuildinfo.json` 进 dist 后，`env -u PYTHONPATH kfc.dist/kfc.bin --help` 在干净环境**完整输出 CLI**(assemble/cli/login/
  run/backtest/journal/slicetool/tool/engage)、**无 warnings 噪声、无 certifi 报错**(--help 已加载 login 等全部命令模块，证明 certifi
  运行期 import 成功)。Stage C Mac freeze 主路径打通。

**Stage C 剩余**：①✅ Mac Nuitka freeze 调通 + kfc.bin 独立运行(本轮完成)。②**Ubuntu 装 python3.13**(deadsnakes/源码)+ Linux
pykungfu 从 3.12 重编 3.13 + Linux env bootstrap + freeze(Linux 侧 nuitka 也升 4.1.2、复核 mypyc 包 nofollow 是否同样命中)。③双平台
各产 kfc。④kungfubuildinfo.json 生成补进 cmake-js 后置步骤(目前 freeze 后手动 cp 进 dist)。⑤`kfc engage` dev 工具桥接(black/pdm/
scons/nuitka)在冻结版的打包策略另案(非 P1 必需；可能改为 dev 环境另装或 bytecode 方式，而非 Nuitka 编译)。⑥freeze 入口正式化为脚本/
run-conan freeze 子命令。⑦体积优化(910M)留后。
</content>
