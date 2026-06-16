#!/usr/bin/env python3
# Kungfu 4.0 最小点亮：Python 经 yijinjing journal 写一帧 longfist 类型再读回。
# 证明 longfist 跨语言二进制契约 + 共享内存 journal 在 Python 侧可用。
import sys, os, tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + "/../build/Release")
import pykungfu as kf

lf, yjj = kf.longfist, kf.yijinjing
mode, category = lf.enums.mode, lf.enums.category


def field(obj, name):
    v = getattr(obj, name)
    return v() if callable(v) else v


base = tempfile.mkdtemp(prefix="kfv4_journal_")
print("journal dir:", base)

print("step: locator"); sys.stdout.flush()
loc = yjj.locator(base)
print("step: location"); sys.stdout.flush()
home = yjj.location(mode.LIVE, category.SYSTEM, "lightup", "demo", loc)
print("step: home uname =", field(home, "uname")); sys.stdout.flush()
print("step: io_device"); sys.stdout.flush()
dev = yjj.io_device(home)
print("step: io_device ok"); sys.stdout.flush()

# 写：home 向 dest=0 写一帧 Quote。
# 注意：io_device.open_writer 造出的 writer 其 publisher_ 为 null（无 master），
# close_frame 末尾会 publisher_->notify() 空指针崩；故直接构造 writer + noop publisher。
# publisher 间接继承 resource，纯虚函数 is_usable/setup/notify/publish 全要实现，
# 故用 C++ 侧绑定的具体 noop_publisher，而非 Python 子类化抽象基类。
DEST = 0

print("step: writer (direct + noop publisher)"); sys.stdout.flush()
wbus = yjj.bus(False)
pub = yjj.noop_publisher()
# writer(location, dest_id, lazy, publisher, low_latency, bus, page_size)
writer = yjj.writer(home, DEST, True, pub, False, wbus, 0)
print("step: writer ok"); sys.stdout.flush()
print("step: Quote()"); sys.stdout.flush()
q = lf.types.Quote()
print("step: set instrument_id"); sys.stdout.flush()
q.instrument_id = "600000"
print("step: set exchange_id"); sys.stdout.flush()
q.exchange_id = "SSE"
print("step: set last_price"); sys.stdout.flush()
q.last_price = 12.34
print("step: write"); sys.stdout.flush()
t0 = yjj.now_in_nano()
writer.write(t0, q)
print("WROTE Quote @", t0, "instrument=600000 last_price=12.34"); sys.stdout.flush()

# 读：从同一 journal 读回
reader = dev.open_reader_to_subscribe()
reader.join(home, DEST, 0)
read_count = 0
while reader.data_available():
    f = reader.current_frame()
    mt = field(f, "msg_type")
    if mt == 401:  # Quote
        got = f.Quote()
        print(
            "READ BACK Quote: instrument=%s exchange=%s last_price=%s gen_time=%s"
            % (field(got, "instrument_id"), field(got, "exchange_id"),
               field(got, "last_price"), field(f, "gen_time"))
        )
        read_count += 1
    reader.next()

print("frames matched:", read_count)
assert read_count >= 1, "未读回写入的 Quote 帧"
print("OK: Python 经 journal 写读 longfist 类型往返成功")
