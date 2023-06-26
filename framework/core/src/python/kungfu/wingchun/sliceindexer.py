#  SPDX-License-Identifier: Apache-2.0

import asyncio
import importlib.util
import inspect
import functools
import kungfu
import os
import sys

from kungfu.yijinjing import time as kft
from kungfu.wingchun import constants
from kungfu.wingchun import utils
from kungfu.wingchun.constants import *

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing


class SliceIndexer(wc.SliceIndexer):
    def __init__(self, ctx, begin_time, end_time, module_path):
        wc.SliceIndexer.__init__(self, begin_time, end_time)
        ctx.log = ctx.logger
        ctx.begin = begin_time
        ctx.end = end_time
        ctx.utils = utils
        self.ctx = ctx
        self.__init_slice_indexer(module_path)

    def __init_slice_indexer(self, path):
        operator_dir = os.path.dirname(path)
        name_no_ext = os.path.split(os.path.basename(path))
        module_name = os.path.splitext(name_no_ext[1])[0]
        module_spec = importlib.util.spec_from_file_location(module_name, path)
        self._module = importlib.util.module_from_spec(module_spec)
        module_spec.loader.exec_module(self._module)
        self._find_md_slice_location = getattr(
            self._module, "find_md_slice_location", lambda ctx: None
        )
        self._get_md_slice_end_time = getattr(
            self._module, "get_md_slice_end_time", lambda ctx: None
        )
        self._find_operator_slice_location = getattr(
            self._module, "find_operator_slice_location", lambda ctx: None
        )
        self._get_operator_slice_end_time = getattr(
            self._module, "get_operator_slice_end_time", lambda ctx: None
        )

    def __call_proxy(self, func, *args):
        return func(*args)

    def find_md_slice_location(
        self, nano_time, group, name, instrument_id, exchange_id, data_type
    ):
        return self.__call_proxy(
            self._find_md_slice_location,
            self.ctx,
            nano_time,
            group,
            name,
            instrument_id,
            exchange_id,
            data_type,
        )

    def get_md_slice_end_time(
        self, nano_time, group, name, instrument_id, exchange_id, data_type
    ):
        return self.__call_proxy(
            self._get_md_slice_end_time,
            self.ctx,
            nano_time,
            group,
            name,
            instrument_id,
            exchange_id,
            data_type,
        )

    def find_operator_slice_location(self, nano_time, group, name):
        return self.__call_proxy(
            self._find_operator_slice_location, self.ctx, nano_time, group, name
        )

    def get_operator_slice_end_time(self, nano_time, group, name):
        return self.__call_proxy(
            self._get_operator_slice_end_time, self.ctx, nano_time, group, name
        )
