// SPDX-License-Identifier: Apache-2.0
//
// Stage C kfc freeze 独立入口（脱 conan2；对应迁移文档 D6 + Stage C wiring）。
//
// 背景：conan2 移除了独立 `conan package` 本地命令，原 `freeze`→run-conan.js package
// 只触发 `conan build`（占位，不跑 freezer）。本脚本把 freeze 做成 `./kungfu-code freeze`
// 一步可复现：
//   1. staging   : src/include → build/include；确保 build/libs 存在（kfc.spec datas 依赖）
//   2. buildinfo : 确保 build/<type>/kungfubuildinfo.json 存在（缺则用 gen_kungfubuildinfo.py 生成）
//   3. pyinstaller: 在 framework/core 下跑 kfc.spec → dist/kfc（onedir，内容落在 _internal/）
//   4. kfs 合并  : dist/kfs 的 *kfs* 拷进 dist/kfc 后删除 dist/kfs（复刻 conanfile.__run_pyinstaller）
//   5. promote   : dist/kfc/_internal/* → dist/kfc 顶层（Unix 符号链 / Windows 拷贝），
//                  让 app/cli/api 栈对 dist/kfc 扁平布局的假设成立（getKfcDir / require.resolve）
//
// PyInstaller 6.x onedir 把数据/库放进 _internal/，而 kfc exe 运行时也依赖该布局，
// 故不摊平 _internal，只在顶层补一层 promote 视图（kfc exe 自身仍从 _internal 加载）。
// kfc.spec 的 contents_directory='.' 在 MERGE 下实测无效，不走该路。

const fs = require('fs');
const path = require('path');
const { shell } = require('../lib');

const CORE = path.resolve(__dirname, '..'); // framework/core
const isWin = process.platform === 'win32';

function buildType() {
  return shell.getConfigValue('build_type') || 'Release';
}

// 1. staging：kfc.spec 的 datas 引用 build/include 与 build/libs。
function stage() {
  const srcInc = path.join(CORE, 'src', 'include');
  const buildInc = path.join(CORE, 'build', 'include');
  console.log('[freeze] 1/5 staging: src/include → build/include');
  fs.rmSync(buildInc, { recursive: true, force: true });
  fs.cpSync(srcInc, buildInc, { recursive: true });
  // Windows 产 *.lib 进 build/libs；Mac/Linux 常为空，但 spec datas 需目录存在。
  fs.mkdirSync(path.join(CORE, 'build', 'libs'), { recursive: true });
}

// 2. buildinfo：kungfu/__init__ 读它取 version；缺则用与 conanfile 同源的脚本生成。
function ensureBuildInfo(bt) {
  const dir = path.join(CORE, 'build', bt);
  const info = path.join(dir, 'kungfubuildinfo.json');
  if (fs.existsSync(info)) return;
  console.log(`[freeze] 2/5 buildinfo 缺失，生成 → ${info}`);
  fs.mkdirSync(dir, { recursive: true });
  shell.run(
    'pipenv',
    ['run', 'python3', path.join(CORE, '.gyp', 'gen_kungfubuildinfo.py'), info],
    true,
    { cwd: CORE },
  );
}

// 3. pyinstaller：在 core 下跑 kfc.spec（cwd 决定 spec 内 .cmake/.deps/build 等路径解析）。
function runPyinstaller(bt) {
  console.log(`[freeze] 3/5 pyinstaller kfc.spec (CMAKE_BUILD_TYPE=${bt})`);
  fs.rmSync(path.join(CORE, 'dist'), { recursive: true, force: true });
  shell.run(
    'pipenv',
    [
      'run',
      'pyinstaller',
      '--workpath=build',
      '--distpath=dist',
      '--clean',
      '--noconfirm',
      path.join('src', 'python', 'kfc.spec'),
    ],
    true,
    {
      cwd: CORE,
      env: {
        ...process.env,
        CMAKE_BUILD_TYPE: bt,
        KFC_PYI_HOOKS_PATH: path.join(CORE, 'src', 'python', 'pyi-hooks'),
      },
    },
  );
}

// 4. kfs 合并：MERGE 模式下 kfs 与 kfc 共享 _internal，dist/kfs 仅余 kfs 入口，拷进 kfc 后删除。
function mergeKfs() {
  const distKfc = path.join(CORE, 'dist', 'kfc');
  const distKfs = path.join(CORE, 'dist', 'kfs');
  if (!fs.existsSync(distKfs)) return;
  console.log('[freeze] 4/5 合并 kfs → kfc');
  for (const f of fs.readdirSync(distKfs)) {
    if (!f.includes('kfs')) continue;
    const from = path.join(distKfs, f);
    if (!fs.statSync(from).isFile()) continue;
    fs.copyFileSync(from, path.join(distKfc, f));
  }
  fs.rmSync(distKfs, { recursive: true, force: true });
}

// 5. promote：_internal/* 在 dist/kfc 顶层补一层视图，满足 app 栈的扁平布局假设。
function promote() {
  const distKfc = path.join(CORE, 'dist', 'kfc');
  const internal = path.join(distKfc, '_internal');
  if (!fs.existsSync(internal)) {
    console.error(
      `[freeze] 错误：未找到 ${internal}（PyInstaller onedir 布局变化？）`,
    );
    process.exit(1);
  }
  console.log(
    `[freeze] 5/5 promote _internal/* → 顶层（${isWin ? '拷贝' : '符号链'}）`,
  );
  let n = 0;
  for (const entry of fs.readdirSync(internal)) {
    const top = path.join(distKfc, entry);
    if (existsLstat(top)) continue; // 跳过 pyinstaller 已放在顶层的项（kfc/kfs exe 等）
    if (isWin) {
      fs.cpSync(path.join(internal, entry), top, { recursive: true });
    } else {
      // 相对链接（指向同目录下的 _internal/<entry>），便于整个 dist/kfc 搬迁。
      fs.symlinkSync(path.join('_internal', entry), top);
    }
    n++;
  }
  console.log(`[freeze] promote 完成：${n} 项`);
}

function existsLstat(p) {
  try {
    fs.lstatSync(p);
    return true;
  } catch (e) {
    return false;
  }
}

function main() {
  const bt = buildType();
  stage();
  ensureBuildInfo(bt);
  runPyinstaller(bt);
  mergeKfs();
  promote();
  console.log('[freeze] ✅ dist/kfc 就绪（扁平视图 + _internal 真身）');
}

main();
