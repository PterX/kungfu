#  SPDX-License-Identifier: Apache-2.0

import asyncio
import importlib.util
import inspect
from functools import partial
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
        ctx.now = partial(wc.Report.now, self)
        ctx.bookkeeper = self.bookkeeper
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
        self._init = getattr(self._module, "init", lambda ctx: None)
        self._sumerize = getattr(self._module, "sumerize", lambda ctx: None)

        self._on_quote = getattr(self._module, "on_quote", lambda ctx, quote: None)
        self._on_entrust = getattr(
            self._module, "on_entrust", lambda ctx, entrust: None
        )
        self._on_transaction = getattr(
            self._module, "on_transaction", lambda ctx, transaction: None,
        )

        self._on_tree = getattr(self._module, "on_tree", lambda ctx, tree: None)

        self._on_read_synthetic_data = getattr(
            self._module, "on_read_synthetic_data", lambda ctx, synthetic_data: None,
        )

        self._on_write_synthetic_data = getattr(
            self._module, "on_write_synthetic_data", lambda ctx, synthetic_data: None,
        )

        self._on_order = getattr(self._module, "on_order", lambda ctx, order: None)

        self._on_trade = getattr(self._module, "on_trade", lambda ctx, trade: None)

    def init(self):
        self.ctx.bookkeeper = self.bookkeeper
        self._init(self.ctx)

    def sumerize(self):
        self._sumerize(self.ctx)

    def on_quote(self, quote):
        self._on_quote(self.ctx, quote)

    def on_entrust(self, entrust):
        self._on_entrust(self.ctx, entrust)

    def on_transaction(self, transaction):
        self._on_transaction(self.ctx, transaction)

    def on_tree(self, tree):
        self._on_transaction(self.ctx, tree)

    def on_read_synthetic_data(self, synthetic_data):
        self._on_read_synthetic_data(self.ctx, synthetic_data)

    def on_write_synthetic_data(self, synthetic_data):
        self._on_write_synthetic_data(self.ctx, synthetic_data)

    def on_order(self, order):
        self._on_order(self.ctx, order)

    def on_trade(self, trade):
        self._on_trade(self.ctx, trade)
