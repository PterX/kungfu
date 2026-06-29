// SPDX-License-Identifier: Apache-2.0
//
// verify — kungfu 构建链端到端「绿」验证(现代化评估 spec B3)
//
// 把原先靠人工分散敲(freeze → build:core → build:app → 启 app)、无单一判据的
// Phase 1 gate,收敛成「一条命令 + 断言产物」的可复现判据,同时作 CI smoke 基础。
//
// 用法(经起手式钉定 node;直接 `node verify.js` 亦可):
//   ./kungfu-code verify              快速:只断言「已有」产物(dist/kfc 存在 + kfc 可跑 + 版本对)
//   ./kungfu-code verify --full       完整:先 rebuild:core + freeze 再断言(~数百秒,需全工具链)
//   ./kungfu-code verify --with-app   额外断言 build:app 产物(配合 --full 会先构建 app)
//   ./kungfu-code verify --help
//
// 断言对象(全部 grounded 自构建脚本,非臆测):
//   - framework/core/dist/kfc/                     freeze 产物目录(run-freeze.js renameSync 目标)
//   - framework/core/dist/kfc/kfc[.exe]           kfc 可执行(lib/executable.js 解析路径)
//   - `kfc --version` 退出 0 且输出含期望版本   冻结 Python 运行时端到端可跑(运行时冒烟)
//   - framework/app/dist/app/                      (--with-app)build:app webpack 产物
//
// 退出码:全绿 0;任一断言失败 1(fail-fast 打印可复制排查信息)。
// 说明:Electron app 的「交互式启动」无法在无头环境断言,故以 kfc 运行时冒烟 + app 构建
// 产物存在 作为可机器断言的等价判据;真实窗口启动留作人工/CI-display 步骤。

'use strict';

const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const ROOT = __dirname;
const isWin = process.platform === 'win32';

const args = process.argv.slice(2);
if (args.includes('--help') || args.includes('-h')) {
  // 打印文件头部用法块(到第一个空注释行为止)
  const head = fs.readFileSync(__filename, 'utf8').split('\n');
  for (const line of head) {
    if (!line.startsWith('//')) break;
    console.log(line.replace(/^\/\/ ?/, ''));
  }
  process.exit(0);
}
const doFull = args.includes('--full');
const withApp = args.includes('--with-app');

// 期望版本:单一真相源取 lerna.json(组织仓 action-bump-version 维护的版本)
function expectedVersion() {
  const lerna = JSON.parse(fs.readFileSync(path.join(ROOT, 'lerna.json'), 'utf8'));
  return lerna.version;
}

const results = [];
function pass(name, detail) {
  results.push({ name, ok: true, detail: detail || '' });
  console.log(`  ✅ ${name}${detail ? ' — ' + detail : ''}`);
}
function fail(name, detail) {
  results.push({ name, ok: false, detail: detail || '' });
  console.error(`  ❌ ${name}${detail ? ' — ' + detail : ''}`);
}

// 运行一个 pnpm 任务(经当前 node/corepack 派发的 pnpm);失败即抛
function runPnpm(task) {
  console.log(`\n[verify] 构建阶段:pnpm ${task}`);
  const r = spawnSync('pnpm', ['run', task], { cwd: ROOT, stdio: 'inherit', shell: isWin });
  if (r.status !== 0) {
    throw new Error(`pnpm ${task} 失败(exit ${r.status == null ? 'signal ' + r.signal : r.status})`);
  }
}

function main() {
  const version = expectedVersion();
  console.log(`[verify] kungfu 构建链验证 — 期望版本 ${version}`);
  console.log(`[verify] 模式:${doFull ? '完整(先构建)' : '快速(只断言已有产物)'}${withApp ? ' + app' : ''}`);

  // ── 阶段 0:工具链 preflight(只读)────────────────────────────────
  console.log('\n[verify] 阶段 0:工具链 preflight');
  const uv = spawnSync('uv', ['--version'], { encoding: 'utf8', shell: isWin });
  if (uv.status === 0) pass('uv 可用', (uv.stdout || '').trim());
  else fail('uv 可用', 'uv 未找到(Python 构建链依赖 uv);完整构建会失败');

  const nodeVersionFile = path.join(ROOT, '.node-version');
  if (fs.existsSync(nodeVersionFile)) {
    const want = fs.readFileSync(nodeVersionFile, 'utf8').trim();
    const got = process.versions.node;
    if (got === want) pass('node 版本钉定', `v${got}`);
    else fail('node 版本钉定', `当前 v${got} ≠ .node-version ${want}(请经 ./kungfu-code 运行)`);
  }

  // ── 阶段 1:(可选)构建 ──────────────────────────────────────────
  if (doFull) {
    try {
      runPnpm('rebuild:core'); // clean + build:conan C++/wheel/native
      runPnpm('freeze'); // nuitka → framework/core/dist/kfc
      if (withApp) runPnpm('build:app'); // webpack → framework/app/dist/app
    } catch (e) {
      fail('构建阶段', e.message);
      return summarize(); // 构建失败直接收尾,不再断言半成品
    }
    pass('构建阶段', '全链执行完成');
  }

  // ── 阶段 2:kfc 产物断言 ─────────────────────────────────────────
  console.log('\n[verify] 阶段 2:kfc 产物断言');
  const distKfc = path.join(ROOT, 'framework', 'core', 'dist', 'kfc');
  let kfcBin = null;
  if (fs.existsSync(distKfc) && fs.statSync(distKfc).isDirectory()) {
    pass('dist/kfc 目录存在', path.relative(ROOT, distKfc));
    kfcBin = path.join(distKfc, isWin ? 'kfc.exe' : 'kfc');
    if (fs.existsSync(kfcBin) && fs.statSync(kfcBin).isFile()) {
      let detail = path.relative(ROOT, kfcBin);
      if (!isWin) {
        const mode = fs.statSync(kfcBin).mode;
        if (!(mode & 0o111)) { fail('kfc 可执行', `${detail} 缺可执行位`); kfcBin = null; }
        else pass('kfc 可执行存在', detail);
      } else pass('kfc 可执行存在', detail);
    } else {
      fail('kfc 可执行存在', `未找到 ${path.relative(ROOT, kfcBin)}(需先 freeze)`);
      kfcBin = null;
    }
  } else {
    fail('dist/kfc 目录存在', `未找到 ${path.relative(ROOT, distKfc)}(需先 freeze;快速模式请确认已构建)`);
  }

  // ── 阶段 3:kfc 运行时冒烟 ───────────────────────────────────────
  console.log('\n[verify] 阶段 3:kfc 运行时冒烟(kfc --version)');
  if (kfcBin) {
    const r = spawnSync(kfcBin, ['--version'], { encoding: 'utf8' });
    const out = `${r.stdout || ''}${r.stderr || ''}`.trim();
    if (r.status !== 0) {
      fail('kfc --version 退出 0', `exit ${r.status == null ? 'signal ' + r.signal : r.status};输出:${out.slice(0, 200)}`);
    } else if (!out.includes(version)) {
      fail('kfc 版本匹配', `期望含 ${version},实得:${out.slice(0, 200)}`);
    } else {
      pass('kfc 运行时冒烟', `--version 含 ${version}`);
    }
  } else {
    fail('kfc 运行时冒烟', '无可执行 kfc,跳过');
  }

  // ── 阶段 4:(可选)app 构建产物 ─────────────────────────────────
  if (withApp) {
    console.log('\n[verify] 阶段 4:app 构建产物断言');
    const appDist = path.join(ROOT, 'framework', 'app', 'dist', 'app');
    if (fs.existsSync(appDist) && fs.statSync(appDist).isDirectory() && fs.readdirSync(appDist).length > 0) {
      pass('build:app 产物存在', path.relative(ROOT, appDist));
    } else {
      fail('build:app 产物存在', `未找到非空 ${path.relative(ROOT, appDist)}(需先 build:app)`);
    }
  }

  return summarize();
}

function summarize() {
  const failed = results.filter((r) => !r.ok);
  console.log(`\n[verify] 结果:${results.length - failed.length}/${results.length} 通过`);
  if (failed.length) {
    console.error(`[verify] ❌ 验证未通过(${failed.length} 项):${failed.map((r) => r.name).join('、')}`);
    process.exit(1);
  }
  console.log('[verify] ✅ 构建链端到端验证通过');
  process.exit(0);
}

main();
