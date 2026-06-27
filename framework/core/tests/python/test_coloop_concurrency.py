"""KungfuEventLoop(coloop.py) 协程调度的可证伪测试 — 阶段0 测试网。

只测纯 Python 事件循环逻辑：按文件路径载入 coloop.py，绕开 kungfu/__init__.py 的
`import pykungfu`(C++ 扩展)，用 FakeHero 替身驱动事件循环，不依赖真实行情/网关/下单。

坐实 principal-engineer 评审发现的缺陷：

- P0  KungfuEventLoop.post_step 用单槽 self._current + 无条件重新入队作为"协程是否结束"
      的标志位(coloop.py:64-69)，配合 strategy.py __call_proxy 的 `await func; loop._current=None`
      (strategy.py:129-133)。多个并发 async 回调同时在飞时，单槽会被互相覆盖，导致部分协程
      丢失或被重复驱动。
- P2  定时器到期判断用严格小于 `handle._when < now()`(coloop.py:57)：精确等于到期时间的
      定时器当轮不触发，回测离散时间下会延迟一个事件。
"""
import asyncio
import importlib.util
import pathlib

_COLOOP = (
    pathlib.Path(__file__).resolve().parents[2]
    / "src/python/kungfu/yijinjing/practice/coloop.py"
)


def _load():
    spec = importlib.util.spec_from_file_location("kungfu_coloop_under_test", _COLOOP)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


coloop = _load()
KungfuEventLoop = coloop.KungfuEventLoop


class _NullLogger:
    def __getattr__(self, _name):
        return lambda *a, **k: None


class FakeHome:
    uid = 0x12345678
    uname = "test.strategy"


class FakeHero:
    """KungfuEventLoop 需要的最小 C++ hero/apprentice 接口替身。"""

    def __init__(self):
        self._now = 0
        self.live = True
        self.home = FakeHome()

    def now(self):
        return self._now

    def advance(self, ns):
        self._now += int(ns)

    def step(self, num=0):
        # 真实 hero.step 会拉一轮 C++ 事件;测试里事件由 call_proxy 直接投递,这里留空。
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
    """复现 strategy.py __call_proxy 的协程回调投递路径:
        async wrap(): await func(...); loop._current = None  + asyncio.ensure_future
    这是 async 策略回调进入 KungfuEventLoop 的真实路径。
    """

    async def wrap():
        await coro_func(*args)
        loop._current = None

    return asyncio.ensure_future(wrap(), loop=loop)


def drive(loop, max_rounds=100):
    """逐轮 post_step 推进(等价 run() 里反复 post_step),直到无待处理回调或达上限。

    返回实际轮数。达到 max_rounds 仍未排空 => 协程未收敛(疑似泄漏/忙轮询)。
    """
    rounds = 0
    while rounds < max_rounds:
        pending = bool(loop._immediate) or bool(loop._scheduled)
        loop.post_step()
        rounds += 1
        if not pending and not loop._immediate and not loop._scheduled:
            break
    return rounds


class WaitRounds:
    """await 它让出控制 n 次(bare yield),模拟需多轮事件循环推进才完成的协程等待。"""

    def __init__(self, n):
        self.n = n

    def __await__(self):
        for _ in range(self.n):
            yield


def test_single_async_callback_completes():
    """smoke:单个 async 回调应被驱动到完成恰好一次。"""
    loop, hero, ctx = make_loop()
    done = []

    async def cb():
        await WaitRounds(2)
        done.append("cb")

    call_proxy(loop, cb)
    drive(loop)
    assert done == ["cb"], f"单协程未恰好完成一次: {done}"


def test_two_concurrent_async_callbacks_both_complete():
    """P0 复现:两个并发 async 回调(模拟两个 async on_quote 同时在飞)应各自恰好完成一次。

    单槽 _current + 无条件重新入队若有缺陷,会丢失或重复驱动其中一个。
    """
    loop, hero, ctx = make_loop()
    done = []

    async def cb(tag):
        await WaitRounds(2)
        done.append(tag)

    call_proxy(loop, cb, "A")
    call_proxy(loop, cb, "B")
    rounds = drive(loop)
    assert sorted(done) == ["A", "B"], (
        f"并发协程未都恰好完成一次(rounds={rounds}): {done}"
    )


def test_timer_fires_at_exact_due_time():
    """P2 复现:call_at(when) 在 hero.now()==when 那一刻应触发。

    coloop.py:57 用 `handle._when < now()`(严格小于),到期时刻当轮不触发。
    """
    loop, hero, ctx = make_loop()
    fired = []
    when = 1000
    loop.call_at(when, lambda: fired.append(hero.now()))
    hero._now = when  # 引擎时间恰好推进到到期时刻
    loop.post_step()
    assert fired == [when], (
        f"到期时刻定时器未触发(now={hero.now()}, when={when}): {fired}"
    )
