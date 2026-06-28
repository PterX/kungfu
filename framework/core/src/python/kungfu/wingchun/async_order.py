#  SPDX-License-Identifier: Apache-2.0
"""协程下单 await 原语：事件驱动 + 可选超时。

替代旧 AsyncOrderAction 的「永不完成 future + 每轮重新轮询订单状态」忙轮询模型——后者在
事件循环不重复入队已跑 handle 后会死锁(见 yijinjing/practice/coloop.py 修正与
tests/python/test_coloop_concurrency.py)。

本模块刻意**不依赖 kungfu C++ binding**(longfist / wingchun)：它只接收 order_id / status /
status_set 这些纯值，由 strategy.py 在 on_order / on_trade 回调里把 longfist 订单字段喂进来
(registry.resolve)。这样下单 await 的核心逻辑可脱离 pykungfu 独立单测。
"""


class OrderTimeout(Exception):
    """await 下单在超时窗口内订单未到终态。"""


class OrderFutureRegistry:
    """order_id -> 等待者列表。订单更新事件到来时，对命中终态集合的等待者 set_result。

    等待者三元组：(status_set, future, timer_handle)。timer_handle 为该等待者的超时定时器
    (可为 None)，resolve / 超时命中时互相取消，避免悬挂回调。
    """

    def __init__(self):
        self._pending = {}

    def register(self, order_id, status_set, future, timer_handle=None):
        self._pending.setdefault(order_id, []).append(
            (status_set, future, timer_handle)
        )

    def resolve(self, order_id, status, order=None):
        """订单更新回调里调用：对 status 命中其 status_set 的等待者 set_result(order)。

        对该 order_id 没有等待者时是无害 no-op(绝大多数订单更新都不在等待中)。
        """
        waiters = self._pending.get(order_id)
        if not waiters:
            return
        remaining = []
        for status_set, future, timer in waiters:
            if future.done():
                continue  # 已被超时/取消解决，丢弃
            if status in status_set:
                if timer is not None:
                    timer.cancel()
                future.set_result(order)
            else:
                remaining.append((status_set, future, timer))
        if remaining:
            self._pending[order_id] = remaining
        else:
            self._pending.pop(order_id, None)

    def discard(self, order_id, future):
        """移除某个具体等待者(超时/取消后清理，避免 dead entry 泄漏)。"""
        waiters = self._pending.get(order_id)
        if not waiters:
            return
        remaining = [(s, f, t) for (s, f, t) in waiters if f is not future]
        if remaining:
            self._pending[order_id] = remaining
        else:
            self._pending.pop(order_id, None)

    def cancel_all(self, exc=None):
        """停机/清理：对所有未决等待者取消定时器并 cancel/set_exception，防 future 泄漏。"""
        for waiters in self._pending.values():
            for _status_set, future, timer in waiters:
                if timer is not None:
                    timer.cancel()
                if future.done():
                    continue
                if exc is not None:
                    future.set_exception(exc)
                else:
                    future.cancel()
        self._pending.clear()

    def pending_count(self):
        return sum(len(v) for v in self._pending.values())


class AsyncOrderAction:
    """await 它直到订单到达 status_set 中的某个(终)态，或超时(若设)。

    用法(strategy.__async_insert_order)::

        order_id = ctx.insert_order(...)
        action = AsyncOrderAction(
            ctx.loop, registry, order_id, status_set,
            timeout_ns=timeout_ns,
            current_status=已在 book 则取当前状态,  # 快速路径
            current_order=对应 order 对象,
        )
        await action
        return ctx.book.orders[order_id]

    current_status 处理「下单后订单已在 book 且已终态(极快成交/同步返回)」：此时直接 resolve，
    避免错过事件后永久等待。
    """

    def __init__(
        self,
        loop,
        registry,
        order_id,
        status_set,
        timeout_ns=None,
        current_status=None,
        current_order=None,
    ):
        self._loop = loop
        self._order_id = order_id
        self._registry = registry
        self._future = loop.create_future()

        # 快速路径：下单时订单已终态，直接 resolve，不入 registry / 不设超时。
        if current_status is not None and current_status in status_set:
            self._future.set_result(current_order)
            return

        timer = None
        if timeout_ns is not None:
            timer = loop.call_at(loop.time() + int(timeout_ns), self._on_timeout)
        registry.register(order_id, status_set, self._future, timer)

    def _on_timeout(self):
        if not self._future.done():
            self._future.set_exception(
                OrderTimeout(f"order {self._order_id} not terminal within timeout")
            )
        # 从 registry 移除本等待者，避免超时后 dead entry 泄漏。
        self._registry.discard(self._order_id, self._future)

    def __await__(self):
        return self._future.__await__()
