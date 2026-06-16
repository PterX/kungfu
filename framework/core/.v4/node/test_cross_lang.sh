#!/usr/bin/env bash
# Kungfu 4.0 Step 3 跨语言点亮编排器：Python 经 journal 写一帧 Quote → Node 读回同一帧。
# 证明 C++(libkungfu) / Python(pykungfu) / Node(kungfu_node) 共享同一条 yijinjing journal。
set -euo pipefail

V4_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PY=/opt/homebrew/opt/python@3.13/bin/python3.13
NODE=/opt/homebrew/opt/node@22/bin/node

BASE="$(mktemp -d -t kfv4_xlang_XXXXXX)"
export KFV4_JOURNAL_DIR="$BASE"
# Node 端 Reader 只认 KF_RUNTIME_DIR(见 kungfu_node_lightup.cpp 注释)，与写入目录对齐。
export KF_RUNTIME_DIR="$BASE"
echo "=== 共享 journal 目录：$BASE ==="

echo "--- [1/2] Python 写入 ---"
arch -arm64 "$PY" "$V4_DIR/python/test_journal_roundtrip.py"

echo "--- [2/2] Node 读取 ---"
arch -arm64 "$NODE" "$V4_DIR/node/test_journal_read.js"

echo "=== 跨语言点亮通过：Python 写、Node 读，同一条 journal ==="
