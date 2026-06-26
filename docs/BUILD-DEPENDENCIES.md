# Kungfu v4 编译依赖

记录 kungfu v4(`@kungfu-trader/kungfu-core` 3.2→4.0.0-alpha 起)+ 其底层 `@kungfu-trader/libnode` 的本地编译依赖与各平台工具链。基于 2026-06 在 Mac / Linux(agent-120)/ Windows(DARKHERO)三平台实际本地构建 libnode v22.22.3 全平台产物的经验整理。

## 1. 运行时基线

- **libnode = Node v22.22.3 LTS('Jod')**。kungfu-core 的 C++/Python/Node 绑定都链接这份 libnode(单进程 polyglot 零拷贝的基础)。
- libnode 以 **node-pre-gyp** 分发:npm「薄壳包」(JS + .gyp 脚本)+ 各平台预编译二进制(`libnode.<abi>.dylib/.so/.dll` + headers + `link_node.node` + `libnodebuildinfo.json`)。
  - 薄壳 npm registry:私有 **Verdaccio**(`http://192.168.100.222:4873/`,kungfu 各机统一 registry)及 GitHub Packages(官方)。
  - 二进制 host:`binary.host = https://prebuilt.libkungfu.cc`(= AWS S3 `kungfu-prebuilt`,CN `cn-northwest-1`),key 布局 `libnode/v{major}/v{version}/libnode-v{version}-{platform}-{arch}-Release.tar.gz`(+ `.md5-checksum`)。

## 2. kungfu-core 本地构建依赖

| 依赖 | 说明 |
|---|---|
| Node(构建驱动) | 任意较新 Node 即可跑 `.gyp/*.js` 脚本(实测 v24);**注意与 libnode 的 v22 ABI 区分**——kungfu_node 走 N-API(ABI 稳定),link 的是 libnode。 |
| Python **带 distutils** | node-gyp(老版,gyp 用 `distutils`)需要。**Python 3.12+ 已移除 distutils** → 必须用 **Python ≤3.11**(实测 `PYTHON=/opt/homebrew/bin/python3.11`)或装 setuptools 垫片。Mac 3.14 / Win 3.14 默认会 `ModuleNotFoundError: distutils`。 |
| pipenv Python 环境 | pykungfu wheel 走 pipenv(`Pipfile`,Python 3.13);`yarn build:core` 会自动 `pipenv install` 拉 poetry/build 工具。 |
| yarn deps(走 Verdaccio) | `yarn install`;若 lockfile 含 `registry.nlark.com` 解析不了的机器,删 `yarn.lock` 让其按 Verdaccio 全新解析。 |
| node-addon-api ^8 | kungfu_node / link_node 的 N-API 头;须在可解析的 node_modules(yarn.lock 应含)。 |
| conan2 + CMake + cmake-js | C++ 内核(libkungfu / pykungfu / kungfu_node / kungfu_electron)构建链。 |

构建命令:`yarn workspace @kungfu-trader/kungfu-core build`(= `node .gyp/run-build.js build`,conan + cmake-js)。验证产物:`build/Release/{kungfu_node,kungfu_electron}.node`、`build/python/dist/kungfu-*.whl`、`libkungfu.dylib/.so/.dll`。

### 已知坑（kungfu-core）

- **`setup.py` install_requires**:依赖约束无下限(如 `*`)时 poetry `constraint.min` 为 `None`,会生成非法的 `certifi>=None` 致 `bdist_wheel` 失败 → 已修为「无 min 时输出裸包名」(`src/python/setup.py`)。
- **clean rebuild 触发 pipenv re-lock 在 pandas 失败**:`yarn rebuild:core` 的 clean 会重 lock,pandas 在 Python 3.13 下从源构建失败;增量构建或 `touch Pipfile.lock` 跳过 re-lock 可正常出包(Python 工具链脆弱点,待治理)。
- **版本号**:走 GitHub workflow 的固定流程升(`Admin - Bump Major Version` → `kungfu-trader/action-bump-version`,lerna `version premajor --preid alpha`);**不手改 package.json**。本地复刻同逻辑:`lerna version premajor --yes --no-push --preid alpha`(全 23 包 + lerna.json 一起升)。

## 3. libnode 本地构建依赖（各平台）

libnode 仓 `node` 子模块 pin 到对应 Node tag(v4 线 = **v22.22.3**;`.gitmodules` tag 同步)。从 committed 状态即可复现构建。

| 平台 | 关键依赖 |
|---|---|
| **通用** | node 子模块 = v22.22.3 源;Python(node 自身 configure 用 gyp-next 可 3.12+;link_node 走老 node-gyp 需 distutils,见上);`yarn install`(走 Verdaccio) |
| **macOS** (darwin-arm64) | Xcode CLT(cc/c++);`yarn make`(node `./configure --shared` + `make -j`)→ `yarn build` → `yarn package` |
| **Linux** (x64) | gcc/g++、make、python3(带 distutils,如 Ubuntu `python3-distutils`);同 make→build→package |
| **Windows** (win32-x64) | **VS 2022 BuildTools** + **nasm**(openssl 汇编必需,`choco install nasm`)+ **ClangCL toolset**(node v22 `common.gypi` 在 `clang==1` 下用 `msbuild_toolset: ClangCL`);`vcbuild.bat dll x64 release` 编 node 本体 |

构建序列(各平台一致):`yarn make`(编 node 本体)→ `KF_SKIP_MAKE_LIBNODE=true yarn build`(binding.gyp 的 link_node + dist 组装)→ `yarn package`(node-pre-gyp 打 tarball)。

### 已知坑（libnode 各平台 —— 重要）

- **binding.gyp `git` target 跑 `git submodule update --init`**:若 node 源是 rsync/解压来的(非注册子模块)会冲突失败。本地构建时把 `.gyp/gyp_action_git.py` 改 no-op(node 源已就位则跳过)。
- **跨平台传 node 源**:Mac → 其它机用 tar/rsync 时,**macOS 会带进大量 `._*` AppleDouble 资源叉文件**(实测 32520 个),Windows 上被 icu 的 `*.cpp` glob 当源码编 → `C2018 未知字符` 等。**必须 `COPYFILE_DISABLE=1 tar`,或解压后删所有 `._*`**。
- **Windows 长路径**:node 源有 >260 字符深路径,须开 `HKLM\...\FileSystem\LongPathsEnabled=1` + `git config core.longpaths true`。
- **Windows `node\deps\v8\third_party\ittapi` 扫描 EPERM**:node-gyp configure 的 `node/**` glob 在该目录 readdir EPERM(node 本体已编、link_node 不需它)→ 删除 ittapi 目录即可让 glob 跳过。
- **Windows ClangCL toolset 缺(MSB8020)**:VS BuildTools 未装 ClangCL MSBuild 集成时 node-gyp build 报 `MSB8020 找不到 ClangCL`。两条路:① 装 VS 组件 `Microsoft.VisualStudio.Component.VC.Llvm.Clang(Toolset)`;② **若只为 link_node(空 N-API stub)**,用 `clang-cl.exe`(LLVM)直编绕开 node-gyp MSBuild:
  ```
  clang-cl /LD /std:c++17 /EHsc /MD /O2 ^
    /I node_modules\node-addon-api /I node\src /I node\deps\v8\include /I node\deps\uv\include ^
    /DNAPI_VERSION=8 /DNODE_GYP_MODULE_NAME=link_node /DBUILDING_NODE_EXTENSION ^
    src\cpp\link.cc /link dist\node\libnode.lib /OUT:dist\node\link_node.node /DLL
  ```
  (在 vcvars64 环境里跑,取 SDK 库路径)。
- **Windows 构建输出在 `node\out\Release`**(含 `libnode.dll` / `node.lib`),不是 `node\Release`。
- **PowerShell over SSH**:执行策略禁 .ps1 时,用 `powershell -Command -`(读 stdin)或 cmd.exe 跑 `.cmd`;复杂脚本经授权后用 `-ExecutionPolicy Bypass -File`。`Start-Process -RedirectStandardOutput` 会缓冲、构建中看不到实时输出 → 用 `Tee-Object` 或 cmd `> log 2>&1`。

## 4. 发布 libnode（参考）

- 二进制:各平台本地构建出 `*-Release.tar.gz` + `.md5-checksum`(33 字节,hash + 换行)→ `aws s3 cp` 到 `s3://kungfu-prebuilt/libnode/v{major}/v{version}/`(CN 区凭据)→ 经 `prebuilt.libkungfu.cc` CDN 服务。Linux/Win 无 aws 凭据时,tarball 中转到有凭据的机器(Mac)再传。
- npm 壳:`npm publish`;**注意 `.npmrc` 的 `@kungfu-trader:registry` 会静默覆盖 `--registry`**(默认发到 GitHub Packages)。发私有 Verdaccio 须 `npm publish --@kungfu-trader:registry=http://192.168.100.222:4873/`;验证必须直接 `curl` Verdaccio API,不能信 npm 的 scoped 命令。
- 正式公开发布走官方 CI(`release-new-version.yml`,PR 合 `release/v*.x` → 全平台 + US/CN S3 + npm),前提是 node 子模块 pin 已提交为目标版本。

## 5. Electron app 构建 + electron 镜像/缓存（v4，重要）

- **electron 版本统一 = 42.5.0**(最新 stable;可人 2026-06-26 定)。声明两处:`framework/core/package.json` devDep(构建源,run-conan.js 读它给 cmake-js)+ `framework/app/package.json`(运行时)。
  - electron 42 自带 **node 24.17**,与 libnode v22.22.3(kfc 运行时)node 大版本不同;N-API(NAPI_VERSION=8)ABI 稳定故 kungfu_electron.node 跨 node 版本通用,功能无碍。
  - node-22 线最后一个 electron 是 38;39+ 都是 node 24。若要 node 对齐则选 38,要最新则 42。
- **electron 头(cmake-js 编 kungfu_electron 用)**:**npmmirror 头镜像不稳**(`SHASUMS256.txt` 504、URL 拼接易畸形)。改用**官方 `https://artifacts.electronjs.org`**(走本机代理可达)——即**不设** `ELECTRON_MIRROR` 让 cmake-js 用默认 dist。
- **electron 二进制(`yarn app` 运行时,@electron/get 下 ~120MB)**:CN CDN(github/npmmirror)持续超时。已**缓存到 LAN 大文件缓存**:
  - URL:`http://192.168.100.222:8088/electron/v42.5.0/`(zip + SHASUMS256.txt;289 MB/s LAN)。
  - 发布脚本:`infra/download-accelerators/scripts/publish-large-file-cache.sh --apply --namespace electron/v<ver> -- <zip> <SHASUMS256.txt>`(Atlas 仓)。
  - **未来构建用 LAN 取 electron 二进制**:`ELECTRON_MIRROR=http://192.168.100.222:8088/electron/ ELECTRON_CUSTOM_DIR='v{{ version }}' yarn install`(@electron/get 拼 `<mirror><dir>/electron-v<ver>-<plat>-<arch>.zip`)。
  - 校验:下载的 zip sha256 须与官方 SHASUMS 一致(实测 v42.5.0 darwin-arm64 = `5e392f66…a1e6`)。
- **kfc freeze 布局 ↔ app 消费(Stage C 未完)**:PyInstaller 6.x onedir 把内容放进 `dist/kfc/_internal/`,但 app(`lib/kfc.js`、`framework/api/toolkit/utils.js` 经 `dist/kfc/<X>` 解析 `drone.node`/`kungfubuildinfo.json`/headers)预期**扁平** `dist/kfc/`。
  - `kfc.spec` COLLECT 加 `contents_directory='.'` **实测在 MERGE 下不生效**(`_internal/` 仍在),待解。
  - 临时解锁(gate 验证):freeze 后把 `dist/kfc/_internal/*` 符号链到 `dist/kfc/` 顶层(exe 照用 _internal、app 经符号链找到)。正式 Stage C 应:修好 contents_directory,或在 conanfile freeze 流程加 promote/flatten,或改 app resolver 走 `_internal/`。
- **app 构建/启动**:`yarn build:app`(kungfu-app webpack)→ `yarn app`(= artifact `kfs craft dev`,启 Electron GUI)。
