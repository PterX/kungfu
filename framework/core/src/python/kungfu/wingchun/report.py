#  SPDX-License-Identifier: Apache-2.0

from functools import partial
from abc import ABC, abstractmethod
from datetime import datetime, timedelta
import kungfu
import os
import sys

from functools import lru_cache
from kungfu.console.utils import safe_import
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

    def __bind_on_func(self, func_name):
        if not hasattr(self._module, func_name):
            return
        func = getattr(self._module, func_name)

        def proxy_on_func(lf_data):
            func(self.ctx, lf_data)

        setattr(self, func_name, proxy_on_func)

    def __init_report(self, path):
        report_dir = os.path.dirname(path)
        name_no_ext = os.path.split(os.path.basename(path))
        sys.path.insert(0, os.path.relpath(report_dir))
        module_name = os.path.splitext(name_no_ext[1])[0]
        self._module = safe_import(module_name)
        self._init = getattr(self._module, "init", lambda ctx: None)
        self._sumerize = getattr(self._module, "sumerize", lambda ctx: None)

        for func_name in [
            "on_quote",
            "on_entrust",
            "on_transaction",
            "on_tree",
            "on_depth",
            "on_tick",
            "on_read_synthetic_data",
            "on_write_synthetic_data",
            "on_order",
            "on_trade",
        ]:
            self.__bind_on_func(func_name)

    def init(self):
        self.ctx.bookkeeper = self.bookkeeper
        self.ctx.report_config = self.config
        self._init(self.ctx)

    def sumerize(self):
        self._sumerize(self.ctx)


class PeriodResult(ABC):
    def __init__(self, begin_time: int) -> None:
        self._last_now = begin_time

    def __init_subclass__(cls, **kwargs):
        def init_decorator(previous_init):
            def new_init(self, *args, **kwargs):
                previous_init(self, *args, **kwargs)
                if type(self) is cls:
                    self.__post_init__()

            return new_init

        cls.__init__ = init_decorator(cls.__init__)

    def __post_init__(self):
        self.init_state()

    def update(self, nano_now: int, bookkeeper: wc.Bookkeeper, **kargs):
        asset = self.get_strategy_asset(bookkeeper)
        n_period = self._n_period_cross(nano_now)
        if n_period == 0:
            return
        for index in reversed(range(n_period)):
            if index == 0:
                self.append_state(**self.evaluate_state(nano_now, asset, **kargs))
            else:
                self.append_default_state()

    def _n_period_cross(self, nano_now: int) -> int:
        if not hasattr(self, "_last_now"):
            self._last_now = nano_now
        last_datetime = kft.to_datetime(self._last_now)
        now_datetime = kft.to_datetime(nano_now)

        n_period = 0
        next_datetime = self.next_period(last_datetime)
        while next_datetime < now_datetime:
            n_period += 1
            next_datetime = self.next_period(next_datetime)
        self._last_now = nano_now
        return n_period

    @lru_cache()
    def get_strategy_asset(self, bookkeeper: wc.Bookkeeper) -> lf.types.Asset:
        books = bookkeeper.get_books()
        for book in books.values():
            asset = book.asset
            if asset.ledger_category == LedgerCategory.Strategy:
                return asset
        return None

    @staticmethod
    @abstractmethod
    def begin_period(date_time: datetime) -> datetime:
        date_time = date_time.replace(hour=0, minute=0, second=0, microsecond=0)
        return date_time

    @staticmethod
    @abstractmethod
    def period_delta() -> timedelta:
        return timedelta(days=1)

    @staticmethod
    @abstractmethod
    def is_trading_time(date_time: datetime) -> bool:
        return True

    @classmethod
    def next_period(cls, date_time: datetime) -> datetime:
        date_time = cls.begin_period(date_time)
        date_time += cls.period_delta()
        while not cls.is_trading_time(date_time):
            date_time += cls.period_delta()
        return date_time

    @classmethod
    def n_period_between(cls, begin: datetime, end: datetime) -> int:
        n_period = 0
        next_datetime = cls.next_period(begin)
        while next_datetime < end:
            n_period += 1
            next_datetime = cls.next_period(next_datetime)
        return n_period

    @classmethod
    def annual_periods(cls, begin: datetime) -> int:
        year_begin = begin.replace(
            month=1, day=1, hour=0, minute=0, second=0, microsecond=0
        )
        year_end = year_begin.replace(year=year_begin.year + 1)
        return cls.n_period_between(year_begin, year_end)

    @abstractmethod
    def init_state(self):
        pass

    @abstractmethod
    def append_default_state(self):
        pass

    @abstractmethod
    def append_state(self, **kargs):
        pass

    @abstractmethod
    def evaluate_state(self, nano_now: int, book: wc.Book, **kargs):
        pass
