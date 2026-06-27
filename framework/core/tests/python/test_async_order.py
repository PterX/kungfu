"""async_order.py 事件驱动下单 await 的可证伪测试。

文件路径载入 async_order.py + coloop.py(绕开 kungfu/__init__.py 的 import pykungfu),
FakeHero 驱动 KungfuEventLoop，不依赖真实行情/网关/binding。
"""
import asyncio
import importlib.util
import pathlib

_BASE = pathlib.Path(__file__).resolve().parents[2] / "src/python/kungfu"


def _load(relpath, name):
    spec = importlib.util.spec_from_file_location(name, _BASE / relpath)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


coloop = _load("yijinjing/practice/coloop.py", "kungfu_coloop_for_asyncorder")
async_order = _load("wingchun/async_order.py", "kungfu_async_order_under_test")
KungfuEventLoop = coloop.KungfuEventLoop
AsyncOrderAction = async_order.AsyncOrderAction
OrderFutureRegistry = async_order.OrderFutureRegistry
OrderTimeout = async_order.OrderTimeout

TERMINAL = {"Filled", "Cancelled", "Error"}


class _NullLogger:
    def __getattr__(self, _name):
        return lambda *a, **k: None


class FakeHome:
    uid = 0x1
    uname = "t"


class FakeHero:
    def __init__(self):
        self._now = 0
        self.live = True
        self.home = FakeHome()

    def now(self):
        return self._now

    def advance(self, ns):
        self._now += int(ns)

    def step(self, num=0):
        pass

    def pre_setup(self):
        pass

    def setup(self):
        pass

    def on_exit(self):
        pass

    def get_home_uid(self):
        return self.home.uid

    def get_home_uname(self):
        return self.home.uname

    def get_begin_time(self):
        return 0

    def get_end_time(self):
        return 0


class FakeCtx:
    def __init__(self):
        self.logger = _NullLogger()
        self.loop = None


def make_loop():
    hero = FakeHero()
    ctx = FakeCtx()
    loop = KungfuEventLoop(ctx, hero)
    ctx.loop = loop
    return loop, hero, ctx


def call_proxy(loop, coro_func, *args):
    async def wrap():
        await coro_func(*args)

    return asyncio.ensure_future(wrap(), loop=loop)


def drive(loop, max_rounds=20):
    rounds = 0
    while rounds < max_rounds:
        pending = bool(loop._immediate) or bool(loop._scheduled)
        loop.post_step()
        rounds += 1
        if not pending and not loop._immediate and not loop._scheduled:
            break
    return rounds


def test_resolves_when_order_reaches_terminal_status():
    """on_order 回调 resolve 终态 => await ctx.buy() 拿到订单(事件驱动,无忙轮询)。"""
    loop, hero, ctx = make_loop()
    reg = OrderFutureRegistry()
    got = []

    async def cb():
        got.append(await AsyncOrderAction(loop, reg, 1, TERMINAL))

    call_proxy(loop, cb)
    drive(loop)
    assert got == [], "未成交应挂起"
    reg.resolve(1, "Filled", order={"id": 1, "status": "Filled"})  # 模拟 on_order
    drive(loop)
    assert got == [{"id": 1, "status": "Filled"}], got
    assert reg.pending_count() == 0


def test_non_terminal_update_keeps_waiting():
    """非终态更新不应 resolve(部分成交中等)。"""
    loop, hero, ctx = make_loop()
    reg = OrderFutureRegistry()
    got = []

    async def cb():
        got.append(await AsyncOrderAction(loop, reg, 1, TERMINAL))

    call_proxy(loop, cb)
    drive(loop)
    reg.resolve(1, "Submitted")  # 非终态
    drive(loop)
    assert got == [], f"非终态不应完成: {got}"
    assert reg.pending_count() == 1


def test_fast_path_already_terminal():
    """下单时订单已终态(极快成交) => 立即 resolve,不入 registry,不错过事件。"""
    loop, hero, ctx = make_loop()
    reg = OrderFutureRegistry()
    got = []

    async def cb():
        got.append(
            await AsyncOrderAction(
                loop, reg, 1, TERMINAL, current_status="Filled", current_order={"id": 1}
            )
        )

    call_proxy(loop, cb)
    drive(loop)
    assert got == [{"id": 1}], got
    assert reg.pending_count() == 0


def test_timeout_raises_order_timeout():
    """设超时后订单未终态 => await 抛 OrderTimeout(防永久泄漏)。"""
    loop, hero, ctx = make_loop()
    reg = OrderFutureRegistry()
    err = []

    async def cb():
        try:
            await AsyncOrderAction(loop, reg, 1, TERMINAL, timeout_ns=1000)
        except OrderTimeout as e:
            err.append(str(e))

    call_proxy(loop, cb)
    drive(loop)
    assert err == [], "未到超时不应抛"
    hero.advance(1000)  # 引擎时间推进到超时时刻
    drive(loop)
    assert len(err) == 1, err
    assert reg.pending_count() == 0


def test_resolve_cancels_pending_timeout():
    """超时前成交 => resolve 并取消超时定时器;事后越过原超时不应再干扰。"""
    loop, hero, ctx = make_loop()
    reg = OrderFutureRegistry()
    got = []

    async def cb():
        got.append(await AsyncOrderAction(loop, reg, 1, TERMINAL, timeout_ns=1000))

    call_proxy(loop, cb)
    drive(loop)
    reg.resolve(1, "Filled", order="ok")  # 超时前成交
    drive(loop)
    assert got == ["ok"], got
    hero.advance(5000)  # 越过原超时窗口
    drive(loop)
    assert got == ["ok"], f"已成交不应被过期超时干扰: {got}"


def test_cancel_all_prevents_leak():
    """停机 cancel_all => 未决 await 被取消(CancelledError),不泄漏 future。"""
    loop, hero, ctx = make_loop()
    reg = OrderFutureRegistry()
    outcome = []

    async def cb():
        try:
            await AsyncOrderAction(loop, reg, 1, TERMINAL)
            outcome.append("done")
        except asyncio.CancelledError:
            outcome.append("cancelled")

    call_proxy(loop, cb)
    drive(loop)
    assert reg.pending_count() == 1
    reg.cancel_all()  # 停机清理
    drive(loop)
    assert reg.pending_count() == 0
    assert outcome == ["cancelled"], outcome
