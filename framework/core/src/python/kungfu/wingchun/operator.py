#  SPDX-License-Identifier: Apache-2.0

import asyncio
import importlib.util
import inspect
import functools
import kungfu
import os
import sys

from kungfu.console.utils import import_force
from kungfu.yijinjing import time as kft
from kungfu.wingchun import constants
from kungfu.wingchun import utils
from kungfu.wingchun.constants import *

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing


class OpRunner(wc.OpRunner):
    def __init__(self, ctx, mode):
        wc.OpRunner.__init__(
            self, ctx.runtime_locator, ctx.group, ctx.name, mode, ctx.low_latency
        )
        self.ctx = ctx


class Operator(wc.Operator):
    def __init__(self, ctx):
        wc.Operator.__init__(self)
        ctx.log = ctx.logger
        ctx.strftime = kft.strftime
        ctx.strptime = kft.strptime
        ctx.constants = constants
        ctx.utils = utils
        self.ctx = ctx
        self.ctx.books = {}
        self.__init_operator(ctx.path)

    def __init_operator(self, path):
        operator_dir = os.path.dirname(path)
        sys.path.insert(0, operator_dir)
        name_no_ext = os.path.split(os.path.basename(path))
        module_name = os.path.splitext(name_no_ext[1])[0]
        self._module = importlib.import_module(module_name)
        self._pre_start = getattr(self._module, "pre_start", lambda ctx: None)
        self._post_start = getattr(self._module, "post_start", lambda ctx: None)
        self._pre_stop = getattr(self._module, "pre_stop", lambda ctx: None)
        self._post_stop = getattr(self._module, "post_stop", lambda ctx: None)

        self._on_quote = getattr(
            self._module, "on_quote", lambda ctx, quote, location, dest_id: None
        )
        self._on_entrust = getattr(
            self._module, "on_entrust", lambda ctx, entrust, location, dest_id: None
        )
        self._on_transaction = getattr(
            self._module,
            "on_transaction",
            lambda ctx, transaction, location, dest_id: None,
        )

        self._on_tree = getattr(
            self._module, "on_tree", lambda ctx, tree, location: None
        )

        self._on_synthetic_data = getattr(
            self._module,
            "on_synthetic_data",
            lambda ctx, synthetic_data, location, dest_id: None,
        )
        self._on_deregister = getattr(
            self._module, "on_deregister", lambda ctx, deregister, location: None
        )
        self._on_broker_state_change = getattr(
            self._module,
            "on_broker_state_change",
            lambda ctx, broker_state_update, location: None,
        )
        self._on_operator_state_change = getattr(
            self._module,
            "on_operator_state_change",
            lambda ctx, operator_state_update, location: None,
        )

    def __call_proxy(self, func, *args):
        if inspect.iscoroutinefunction(func):

            async def wrap():
                await func(*args)
                self.ctx.loop._current = None

            asyncio.ensure_future(wrap())
        else:
            func(*args)

    def __add_timer(self, nanotime, callback):
        def wrap_callback(event):
            self.__call_proxy(callback, self.ctx, event)

        return self.ctx.wc_context.add_timer(nanotime, wrap_callback)

    def __add_time_interval(self, duration, callback):
        def wrap_callback(event):
            self.__call_proxy(callback, self.ctx, event)

        return self.ctx.wc_context.add_time_interval(duration, wrap_callback)

    def pre_start(self, wc_context):
        self.ctx.wc_context = wc_context
        self.ctx.config = wc_context.config
        self.ctx.now = wc_context.now
        self.ctx.add_timer = self.__add_timer
        self.ctx.add_time_interval = self.__add_time_interval
        self.ctx.clear_timer = wc_context.clear_timer
        self.ctx.subscribe = wc_context.subscribe
        self.ctx.unsubscribe = wc_context.unsubscribe
        self.ctx.subscribe_all = wc_context.subscribe_all
        self.ctx.subscribe_operator = wc_context.subscribe_operator
        self.ctx.update_operator_state = wc_context.update_operator_state
        self.ctx.publish_synthetic_data = wc_context.publish_synthetic_data
        self.ctx.req_deregister = wc_context.req_deregister
        self.ctx.static_data = wc_context.bookkeeper.static_data
        self.__call_proxy(self._pre_start, self.ctx)

    def post_start(self, wc_context):
        self.__call_proxy(self._post_start, self.ctx)

    def pre_stop(self, wc_context):
        self.__call_proxy(self._pre_stop, self.ctx)

    def post_stop(self, wc_context):
        self.__call_proxy(self._post_stop, self.ctx)

    def on_quote(self, wc_context, quote, location, dest_id):
        self.__call_proxy(self._on_quote, self.ctx, quote, location, dest_id)

    def on_entrust(self, wc_context, entrust, location, dest_id):
        self.__call_proxy(self._on_entrust, self.ctx, entrust, location, dest_id)

    def on_transaction(self, wc_context, transaction, location, dest_id):
        self.__call_proxy(
            self._on_transaction, self.ctx, transaction, location, dest_id
        )

    def on_tree(self, wc_context, tree, location, dest_id):
        self.__call_proxy(self._on_transaction, self.ctx, tree, location)

    def on_synthetic_data(self, wc_context, synthetic_data, location, dest_id):
        self.__call_proxy(
            self._on_synthetic_data, self.ctx, synthetic_data, location, dest_id
        )

    def on_deregister(self, wc_context, deregister, location):
        self.__call_proxy(self._on_deregister, self.ctx, deregister, location)

    def on_broker_state_change(self, wc_context, broker_state_update, location):
        self.__call_proxy(
            self._on_broker_state_change, self.ctx, broker_state_update, location
        )

    def on_operator_state_change(self, wc_context, operator_state_update, location):
        self.__call_proxy(
            self._on_operator_state_change, self.ctx, operator_state_update, location
        )
