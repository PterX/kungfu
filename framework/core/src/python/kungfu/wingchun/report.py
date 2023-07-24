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


class Report(wc.Report):
    def __init__(self, ctx):
        wc.Report.__init__(self)
        ctx.log = ctx.logger
        ctx.strftime = kft.strftime
        ctx.strptime = kft.strptime
        ctx.constants = constants
        ctx.utils = utils
        self.ctx = ctx
        self.__init_report(ctx.report)

    def __init_report(self, path):
        report_dir = os.path.dirname(path)
        name_no_ext = os.path.split(os.path.basename(path))
        sys.path.insert(0, os.path.relpath(report_dir))
        module_name = os.path.splitext(name_no_ext[1])[0]
        self._module = import_force(module_name)
        # module_spec = importlib.util.spec_from_file_location(module_name, path)
        # self._module = importlib.util.module_from_spec(module_spec)
        # module_spec.loader.exec_module(self._module)
        # self._pre_stop = getattr(self._module, "pre_stop", lambda ctx: None)
        self._post_stop = getattr(self._module, "post_stop", lambda ctx: None)

        self._on_quote = getattr(self._module, "on_quote", lambda ctx, quote, now: None)
        self._on_entrust = getattr(
            self._module, "on_entrust", lambda ctx, entrust, now: None
        )
        self._on_transaction = getattr(
            self._module,
            "on_transaction",
            lambda ctx, transaction, now: None,
        )

        self._on_tree = getattr(self._module, "on_tree", lambda ctx, tree, now: None)

        self._on_read_synthetic_data = getattr(
            self._module,
            "on_read_synthetic_data",
            lambda ctx, synthetic_data, now: None,
        )

        self._on_write_synthetic_data = getattr(
            self._module,
            "on_write_synthetic_data",
            lambda ctx, synthetic_data, now: None,
        )

        self._on_order = getattr(self._module, "on_order", lambda ctx, order, now: None)

        self._on_trade = getattr(self._module, "on_trade", lambda ctx, trade, now: None)

    def on_quote(self, quote, now):
        self._on_quote(self.ctx, quote, now)

    def on_entrust(self, entrust, now):
        self._on_entrust(self.ctx, entrust, now)

    def on_transaction(self, transaction, now):
        self._on_transaction(self.ctx, transaction, now)

    def on_tree(self, tree, now):
        self._on_transaction(self.ctx, tree, now)

    def on_read_synthetic_data(self, synthetic_data, now):
        self._on_read_synthetic_data(self.ctx, synthetic_data, now)

    def on_write_synthetic_data(self, synthetic_data, now):
        self._on_write_synthetic_data(self.ctx, synthetic_data, now)

    def on_order(self, order, now):
        self._on_order(self.ctx, order, now)

    def on_trade(self, trade, now):
        self._on_trade(self.ctx, trade, now)
