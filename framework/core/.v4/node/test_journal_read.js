#!/usr/bin/env node
// Kungfu 4.0 Step 3 最小点亮：Node.js 经 kungfu_node addon 读 C++/Python 写入的同一条 journal。
// 用法：KFV4_JOURNAL_DIR=<dir> KF_RUNTIME_DIR=<dir> node test_journal_read.js
// 该目录里应已由 Python(Step 2 测试)写入一帧 Quote。Node 端只读，证明三语言共享同一 journal。
// 注意：现有 Node Reader 绑定只认 KF_RUNTIME_DIR(GetDefaultRuntimeLocator)，须与写入目录一致。
const path = require("path");
const assert = require("assert");

const kf = require(path.join(__dirname, "..", "build", "Release", "kungfu_node.node"));

const base = process.env.KFV4_JOURNAL_DIR;
assert(base, "需用环境变量 KFV4_JOURNAL_DIR 指定 Python 写入的 journal 目录");
console.log("journal dir:", base);

// location 与 Python 端一致：mode=live, category=system, group=lightup, name=demo。
const DEST = 0;
const dev = new kf.IODevice(
  { mode: "live", category: "system", group: "lightup", name: "demo" },
  base
);

const reader = dev.openReader();
// join(category, group, name, mode, destId, fromTime)
reader.join("system", "lightup", "demo", "live", DEST, 0n);

let readCount = 0;
while (reader.dataAvailable()) {
  const f = reader.currentFrame();
  if (f.msgType() === 401) {
    // Quote
    const q = f.data();
    console.log(
      `READ BACK Quote: instrument=${q.instrument_id} exchange=${q.exchange_id} ` +
        `last_price=${q.last_price} genTime=${f.genTime()}`
    );
    readCount += 1;
  }
  reader.next();
}

console.log("frames matched:", readCount);
assert(readCount >= 1, "未读回 Python 写入的 Quote 帧");
console.log("OK: Node.js 读到 Python 经同一条 journal 写入的 longfist 类型");
