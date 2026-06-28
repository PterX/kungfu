#  SPDX-License-Identifier: Apache-2.0

import importlib
import kungfu
import os
import sys
from pathlib import Path

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

    def __bind_const_func(self, func_name):
        if not hasattr(self._module, func_name):
            return
        func = getattr(self._module, func_name)

        def proxy_on_func():
            return func(self.ctx)

        setattr(self, func_name, proxy_on_func)

    def __init_slice_indexer(self, path):
        indexer_dir = os.path.dirname(path)
        name_no_ext = os.path.split(os.path.basename(path))
        sys.path.insert(0, os.path.relpath(indexer_dir))

        module_name = os.path.splitext(name_no_ext[1])[0]
        if path.endswith(".so") or path.endswith(".pyd"):
            sys.path.append(str(Path(path).parent))
            module_name = Path(path).stem.split(".")[0]

        self._module = importlib.import_module(module_name)
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
        self._submit_acquire_location = getattr(
            self._module, "submit_acquire_location", lambda ctx, location: None
        )
        self._wait_acquire_location = getattr(
            self._module, "wait_acquire_location", lambda ctx, location: None
        )
        self._submit_release_location = getattr(
            self._module, "submit_release_location", lambda ctx, location: None
        )
        self._wait_release_location = getattr(
            self._module, "wait_release_location", lambda ctx, location: None
        )
        self._sync_save_location = getattr(
            self._module, "sync_save_location", lambda ctx, location: None
        )
        for func_name in ["acquire_lead_ratio", "release_delay_ratio"]:
            self.__bind_const_func(func_name)

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

    def submit_acquire_location(self, location):
        return self.__call_proxy(self._submit_acquire_location, self.ctx, location)

    def wait_acquire_location(self, location):
        return self.__call_proxy(self._wait_acquire_location, self.ctx, location)

    def submit_release_location(self, location):
        return self.__call_proxy(self._submit_release_location, self.ctx, location)

    def wait_release_location(self, location):
        return self.__call_proxy(self._wait_release_location, self.ctx, location)

    def sync_save_location(self, location):
        return self.__call_proxy(self._sync_save_location, self.ctx, location)
