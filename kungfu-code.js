#!/usr/bin/env node
// SPDX-License-Identifier: Apache-2.0
//
// kungfu-code.js — kungfu-code 起手式的「富子集」(L2)。
//
// 三层子集模型:
//   L1  kungfu-code(sh)        bootstrap 简单命令:load env / 检 fnm+uv / 钉 node / 跑 pnpm,
//                              并把富子命令(proxy/config…)委派给本文件。
//   L2  kungfu-code.js(node)   fnm 装好后可用的富命令(node 实现,纯 builtins,无依赖)。
//                              当前:本地缓存/镜像代理配置管理(proxy)。
//   L3  (未来)TUI              复用 kungfu 自身 TUI 基础设施 / 编译产物运行时;直接 import
//                              下面的 readConfig/setKey 等配置读写,不重复实现。
//
// 配置文件在用户全局 ${XDG_CONFIG_HOME:-~/.config}/kungfu/build-local.env:主仓与所有
// git worktree 共用同一份、天然在仓外(开源安全)、内网多机同步一份即可。
'use strict';

const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const KEYS = [
  // 镜像 / 缓存(各上游)
  'FNM_NODE_DIST_MIRROR',
  'ELECTRON_MIRROR',
  'COREPACK_NPM_REGISTRY',
  'UV_DEFAULT_INDEX',
  'UV_PYTHON_INSTALL_MIRROR',
  // 编译参数(按机封顶,防大核机内存 thrash)
  'KUNGFU_BUILD_JOBS',
];

const CONFIG_FILE = path.join(
  process.env.XDG_CONFIG_HOME || path.join(os.homedir(), '.config'),
  'kungfu',
  'build-local.env',
);
const TEMPLATE = path.join(__dirname, 'build-local.env.example');

// ── 可复用配置读写模块(L3 TUI 可直接 require 本文件复用)────────────────────
function readRaw() {
  try {
    return fs.readFileSync(CONFIG_FILE, 'utf8');
  } catch {
    return '';
  }
}

function readConfig() {
  const out = {};
  for (const line of readRaw().split('\n')) {
    const m = line.match(/^\s*(?:export\s+)?([A-Za-z_][A-Za-z0-9_]*)=(.*)$/);
    if (!m) continue;
    let v = m[2].trim();
    if ((v.startsWith("'") && v.endsWith("'")) || (v.startsWith('"') && v.endsWith('"'))) {
      v = v.slice(1, -1);
    }
    out[m[1]] = v;
  }
  return out;
}

function shQuote(v) {
  return "'" + String(v).replace(/'/g, "'\\''") + "'";
}

function ensureDir() {
  fs.mkdirSync(path.dirname(CONFIG_FILE), { recursive: true });
}

function setKey(key, val) {
  ensureDir();
  let text = readRaw();
  if (text && !text.endsWith('\n')) text += '\n';
  const re = new RegExp(`^\\s*(?:export\\s+)?${key}=.*$`, 'm');
  const line = `export ${key}=${shQuote(val)}`;
  text = re.test(text) ? text.replace(re, line) : text + line + '\n';
  fs.writeFileSync(CONFIG_FILE, text);
}

function unsetKey(key) {
  const text = readRaw();
  if (!text) return;
  const re = new RegExp(`^\\s*(?:export\\s+)?${key}=.*\\n?`, 'm');
  fs.writeFileSync(CONFIG_FILE, text.replace(re, ''));
}

module.exports = { CONFIG_FILE, KEYS, readConfig, setKey, unsetKey };

// ── CLI(L1 sh 委派入口)──────────────────────────────────────────────────
function help(cmd) {
  console.error(
    `kungfu-code ${cmd} — 管理本地构建环境配置:镜像/缓存 + 编译参数(用户全局 build-local.env)\n` +
      `  ${cmd} path               打印配置文件路径\n` +
      `  ${cmd} init               从 build-local.env.example 派生配置文件(若不存在)\n` +
      `  ${cmd} edit               用 $EDITOR 打开配置文件(不存在则先从模板创建)\n` +
      `  ${cmd} list               显示各项当前值\n` +
      `  ${cmd} get <KEY>          读取某项\n` +
      `  ${cmd} set <KEY> <VALUE>  设置某项(写入用户全局文件)\n` +
      `  ${cmd} unset <KEY>        移除某项\n` +
      `已知 KEY:${KEYS.join(' ')}\n` +
      `配置在仓外(用户全局),主仓与所有 worktree 共用、内网多机同步一份。`,
  );
}

function main() {
  const argv = process.argv.slice(2);
  const cmd = argv[0];
  if (cmd !== 'proxy' && cmd !== 'config') {
    console.error(`kungfu-code.js: 未知命令 ${cmd || '(空)'}(仅支持 proxy/config)`);
    process.exit(2);
  }
  const sub = argv[1] || 'help';
  switch (sub) {
    case 'path':
      console.log(CONFIG_FILE);
      break;
    case 'init':
      if (fs.existsSync(CONFIG_FILE)) {
        console.error(`已存在,未覆盖:${CONFIG_FILE}`);
      } else {
        ensureDir();
        fs.copyFileSync(TEMPLATE, CONFIG_FILE);
        console.error(`已从模板创建:${CONFIG_FILE}(请编辑或用 set 填值)`);
      }
      break;
    case 'edit': {
      ensureDir();
      if (!fs.existsSync(CONFIG_FILE)) fs.copyFileSync(TEMPLATE, CONFIG_FILE);
      const r = spawnSync(process.env.EDITOR || 'vi', [CONFIG_FILE], { stdio: 'inherit' });
      process.exit(r.status || 0);
      break;
    }
    case 'list': {
      const cfg = readConfig();
      console.log(`# ${CONFIG_FILE}`);
      for (const k of KEYS) console.log(`${k}=${cfg[k] || ''}`);
      break;
    }
    case 'get': {
      const k = argv[2];
      if (!k) {
        console.error('用法: proxy get <KEY>');
        process.exit(2);
      }
      console.log(readConfig()[k] || '');
      break;
    }
    case 'set': {
      const k = argv[2];
      const v = argv[3];
      if (!k || v === undefined) {
        console.error('用法: proxy set <KEY> <VALUE>');
        process.exit(2);
      }
      if (!/^[A-Za-z_][A-Za-z0-9_]*$/.test(k)) {
        console.error(`非法 KEY:${k}`);
        process.exit(2);
      }
      setKey(k, v);
      console.error(`已设置 ${k} 于 ${CONFIG_FILE}`);
      break;
    }
    case 'unset': {
      const k = argv[2];
      if (!k) {
        console.error('用法: proxy unset <KEY>');
        process.exit(2);
      }
      unsetKey(k);
      console.error(`已移除 ${k}`);
      break;
    }
    case 'help':
    case '-h':
    case '--help':
      help(cmd);
      break;
    default:
      console.error(`未知子命令:${sub}(见 ${cmd} help)`);
      process.exit(2);
  }
}

if (require.main === module) main();
