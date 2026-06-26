// SPDX-License-Identifier: Apache-2.0
//
// Stage C kfc freeze 独立入口（脱 conan2；对应迁移文档 D6 + Stage C wiring）。
//
// 背景：conan2 移除了独立 `conan package` 本地命令，原 `freeze`→run-conan.js package
// 只触发 `conan build`（占位，不跑 freezer）。本脚本把 freeze 做成 `./kungfu-code freeze`
// 一步可复现，并据 `config.freezer` 选择 Nuitka（默认）或 PyInstaller（fallback）。
//
// 两条 freezer 路径（可人 2026-06-17 决策 Nuitka 2.x，不行回退 PyInstaller；
// .v4 线 Nuitka 三平台已跑通，见 docs/conan2-migration.md §4c）：
//
// - nuitka（默认）：编译成 C，产物 kfc.dist 本就扁平（无 _internal），移到 dist/kfc 即可，
//   不需要 promote。kfc.py 内嵌 nuitka-project 选项（--standalone 等）。Nuitka 只跟随
//   kfc.py 的 import，故 app/electron 侧 node native（drone.node / kungfu_node.node /
//   kungfu_electron.node）需 freeze 后从 build/<type> 补拷（kfc python 进程不 import 它们）。
// - pyinstaller（fallback）：onedir 把数据/库放进 _internal/，而 app 栈假设 dist/kfc 扁平，
//   故 freeze 后 promote（_internal/*→顶层，Unix 符号链/Win 拷贝）。

const fs = require('fs');
const path = require('path');
const { shell } = require('../lib');

const CORE = path.resolve(__dirname, '..'); // framework/core
const isWin = process.platform === 'win32';

function buildType() {
  return shell.getConfigValue('build_type') || 'Release';
}

function freezer() {
  return shell.getConfigValue('freezer') || 'nuitka';
}

// kfc.spec datas 引用 build/include 与 build/libs（仅 pyinstaller 路径需要）。
function stage() {
  const srcInc = path.join(CORE, 'src', 'include');
  const buildInc = path.join(CORE, 'build', 'include');
  console.log('[freeze] staging: src/include → build/include');
  fs.rmSync(buildInc, { recursive: true, force: true });
  fs.cpSync(srcInc, buildInc, { recursive: true });
  fs.mkdirSync(path.join(CORE, 'build', 'libs'), { recursive: true });
}

// kungfu/__init__ 读 pykungfu 同目录的 kungfubuildinfo.json 取 version；缺则生成。
function ensureBuildInfo(bt) {
  const dir = path.join(CORE, 'build', bt);
  const info = path.join(dir, 'kungfubuildinfo.json');
  if (fs.existsSync(info)) return info;
  console.log(`[freeze] buildinfo 缺失，生成 → ${info}`);
  fs.mkdirSync(dir, { recursive: true });
  shell.run(
    'uv',
    ['run', '--frozen', 'python3', path.join(CORE, '.gyp', 'gen_kungfubuildinfo.py'), info],
    true,
    { cwd: CORE },
  );
  return info;
}

// app/electron 侧 node native：kfc python 进程不 import，Nuitka 不带，需从 build/<type> 补拷。
// app 栈通过 @kungfu-trader/kungfu-core/dist/kfc/<x> 解析它们（getKfcDir / webpack require.resolve）。
const APP_NATIVE = [
  'drone.node',
  'kungfu_node.node',
  'kungfu_electron.node',
  'link_node.node',
];

function copyAppNative(bt) {
  const rel = path.join(CORE, 'build', bt);
  const distKfc = path.join(CORE, 'dist', 'kfc');
  let n = 0;
  for (const f of APP_NATIVE) {
    const from = path.join(rel, f);
    if (!fs.existsSync(from)) continue;
    fs.copyFileSync(from, path.join(distKfc, f));
    n++;
  }
  console.log(`[freeze] 补拷 app native：${n}/${APP_NATIVE.length} 项`);
}

// ----------------------------------------------------------------- nuitka

function freezeNuitka(bt) {
  const rel = path.join(CORE, 'build', bt); // 三 native（pykungfu/libkungfu/libnode）同目录
  const out = path.join(CORE, 'build', 'kfc-nuitka');
  const distKfc = path.join(CORE, 'dist', 'kfc');
  const info = ensureBuildInfo(bt);

  console.log(`[freeze] nuitka kfc.py（PYTHONPATH=${path.relative(CORE, rel)}）`);
  fs.rmSync(out, { recursive: true, force: true });
  shell.run(
    'uv',
    [
      'run',
      '--frozen',
      'python',
      '-m',
      'nuitka',
      '--output-dir=build/kfc-nuitka',
      `--include-data-files=${info}=kungfubuildinfo.json`,
      path.join('src', 'python', 'kfc.py'),
    ],
    true,
    { cwd: CORE, env: { ...process.env, PYTHONPATH: rel } },
  );

  // Nuitka standalone 产物 kfc.dist 本就扁平 → 直接移到 dist/kfc（无 _internal、无 promote）。
  const kfcDist = path.join(out, 'kfc.dist');
  if (!fs.existsSync(kfcDist)) {
    console.error(`[freeze] 错误：未找到 ${kfcDist}（nuitka 产物布局变化？）`);
    process.exit(1);
  }
  fs.rmSync(distKfc, { recursive: true, force: true });
  fs.mkdirSync(path.dirname(distKfc), { recursive: true });
  fs.renameSync(kfcDist, distKfc);

  // nuitka standalone 入口名为 kfc.bin(Unix)/kfc.exe(Win)；app 栈按 'kfc'(Unix)/'kfc.exe'(Win)
  // 定位可执行（framework/api pathConfig/processUtils 的 kfcName）。Win 已匹配；Unix 把
  // kfc.bin 重命名为 kfc（用 rename 不用符号链，保持 nuitka 产物无符号链、electron-builder 打包干净）。
  if (!isWin) {
    const binPath = path.join(distKfc, 'kfc.bin');
    const kfcPath = path.join(distKfc, 'kfc');
    if (fs.existsSync(binPath)) fs.renameSync(binPath, kfcPath);
  }

  copyAppNative(bt);
  console.log('[freeze] ✅ dist/kfc 就绪（nuitka 扁平产物 + app native）');
}

// ------------------------------------------------------------- pyinstaller

function freezePyinstaller(bt) {
  stage();
  ensureBuildInfo(bt);
  console.log(`[freeze] pyinstaller kfc.spec (CMAKE_BUILD_TYPE=${bt})`);
  fs.rmSync(path.join(CORE, 'dist'), { recursive: true, force: true });
  shell.run(
    'uv',
    [
      'run',
      '--frozen',
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
  mergeKfs();
  promote();
  console.log('[freeze] ✅ dist/kfc 就绪（pyinstaller 扁平视图 + _internal 真身）');
}

// MERGE 模式下 kfs 与 kfc 共享 _internal，dist/kfs 仅余 kfs 入口，拷进 kfc 后删除。
function mergeKfs() {
  const distKfc = path.join(CORE, 'dist', 'kfc');
  const distKfs = path.join(CORE, 'dist', 'kfs');
  if (!fs.existsSync(distKfs)) return;
  console.log('[freeze] 合并 kfs → kfc');
  for (const f of fs.readdirSync(distKfs)) {
    if (!f.includes('kfs')) continue;
    const from = path.join(distKfs, f);
    if (!fs.statSync(from).isFile()) continue;
    fs.copyFileSync(from, path.join(distKfc, f));
  }
  fs.rmSync(distKfs, { recursive: true, force: true });
}

// _internal/* 在 dist/kfc 顶层补一层视图，满足 app 栈的扁平布局假设（仅 pyinstaller 路径）。
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
    `[freeze] promote _internal/* → 顶层（${isWin ? '拷贝' : '符号链'}）`,
  );
  let n = 0;
  for (const entry of fs.readdirSync(internal)) {
    const top = path.join(distKfc, entry);
    if (existsLstat(top)) continue; // 跳过 pyinstaller 已放在顶层的项（kfc/kfs exe 等）
    if (isWin) {
      fs.cpSync(path.join(internal, entry), top, { recursive: true });
    } else {
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
  const fz = freezer();
  console.log(`[freeze] freezer=${fz} build_type=${bt}`);
  if (fz === 'nuitka') {
    freezeNuitka(bt);
  } else if (fz === 'pyinstaller') {
    freezePyinstaller(bt);
  } else {
    console.error(`[freeze] 未知 freezer: ${fz}（应为 nuitka 或 pyinstaller）`);
    process.exit(1);
  }
}

main();
