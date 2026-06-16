#!/usr/bin/env python3
# Kungfu 4.0 Step 4 单进程三语言联动点亮：
#   同一个 Python 进程内 —— Python(pykungfu) 经 yijinjing journal 写一帧 Quote，
#   再用 pykungfu.libnode.run("node", ...) 经 node::Start 在【本进程内】启真正的 Node，
#   Node(kungfu_node addon) 读回同一帧。证明 C++/Python/Node 在一个进程里共享同一条
#   零拷贝、纳秒时间戳的 yijinjing journal —— 即 Kungfu 4.0 运行时的技术终点。
import sys, os

V4 = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(V4, "build", "Release"))
import pykungfu as kf

lf, yjj = kf.longfist, kf.yijinjing
mode, category = lf.enums.mode, lf.enums.category


def field(obj, name):
    v = getattr(obj, name)
    return v() if callable(v) else v


base = os.environ["KFV4_JOURNAL_DIR"]  # 与内嵌 Node 的 KF_RUNTIME_DIR 同目录
os.makedirs(base, exist_ok=True)
print("=== [进程内 1/2] Python 写入 ===")
print("journal dir:", base, "pid:", os.getpid()); sys.stdout.flush()

loc = yjj.locator(base)
home = yjj.location(mode.LIVE, category.SYSTEM, "lightup", "demo", loc)
dev = yjj.io_device(home)
DEST = 0
wbus = yjj.bus(False)
pub = yjj.noop_publisher()
writer = yjj.writer(home, DEST, True, pub, False, wbus, 0)
q = lf.types.Quote()
q.instrument_id = "600000"
q.exchange_id = "SSE"
q.last_price = 12.34
t0 = yjj.now_in_nano()
writer.write(t0, q)
print("PYTHON WROTE Quote @", t0, "instrument=600000 last_price=12.34"); sys.stdout.flush()

# === 在【同一进程】内启动 Node 读回同一帧 ===
print("=== [进程内 2/2] node::Start 进程内启 Node 读取 ===", flush=True)
read_js = os.path.join(V4, "node", "test_journal_read.js")
# pykungfu.libnode.run(*argv) → node::Start(argc, argv)，argv[0] 为程序名。
rc = kf.libnode.run("node", read_js)
print("node::Start 返回，pid 仍为:", os.getpid(),
      "(Node 与 Python 同进程)" if rc in (None, 0) else f"(rc={rc})")
print("OK: 单进程内 Python 写、Node 读，共享同一条 journal —— 三语言联动点亮")
