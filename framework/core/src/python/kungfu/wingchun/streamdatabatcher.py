#  SPDX-License-Identifier: Apache-2.0

import asyncio
import importlib
import inspect
import functools
import kungfu
import os
import sys
import numpy as np
import pandas as pd

from kungfu.console.utils import safe_import
from kungfu.yijinjing import time as kft
from kungfu.yijinjing import journal as kfj
from kungfu.wingchun import constants
from kungfu.wingchun import utils
from kungfu.wingchun.constants import *

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing

entrust_column_names = [
    "data_time",
    "instrument_id",
    "exchange_id",
    "instrument_type",
    "price",
    "volume",
    "side",
    "price_type",
    "main_seq",
    "seq",
    "orig_order_no",
    "biz_index",
]

transaction_column_names = [
    "data_time",
    "instrument_id",
    "exchange_id",
    "instrument_type",
    "price",
    "volume",
    "bid_no",
    "ask_no",
    "exec_type",
    "side",
    "main_seq",
    "seq",
    "biz_index",
]

quote_column_names = [
    "data_time",
    "instrument_id",
    "exchange_id",
    "instrument_type",
    "pre_close_price",
    "pre_settlement_price",
    "last_price",
    "volume",
    "turnover",
    "pre_open_interest",
    "open_interest",
    "open_price",
    "high_price",
    "low_price",
    "upper_limit_price",
    "lower_limit_price",
    "close_price",
    "settlement_price",
    "iopv",
    "total_bid_volume",
    "total_ask_volume",
    "total_trade_num",
    "bid_price0",
    "bid_price1",
    "bid_price2",
    "bid_price3",
    "bid_price4",
    "bid_price5",
    "bid_price6",
    "bid_price7",
    "bid_price8",
    "bid_price9",
    "ask_price0",
    "ask_price1",
    "ask_price2",
    "ask_price3",
    "ask_price4",
    "ask_price5",
    "ask_price6",
    "ask_price7",
    "ask_price8",
    "ask_price9",
    "bid_volume0",
    "bid_volume1",
    "bid_volume2",
    "bid_volume3",
    "bid_volume4",
    "bid_volume5",
    "bid_volume6",
    "bid_volume7",
    "bid_volume8",
    "bid_volume9",
    "ask_volume0",
    "ask_volume1",
    "ask_volume2",
    "ask_volume3",
    "ask_volume4",
    "ask_volume5",
    "ask_volume6",
    "ask_volume7",
    "ask_volume8",
    "ask_volume9",
    "trading_phase_code",
]

tree_column_names = [
    "data_time",
    "instrument_id",
    "exchange_id",
    "instrument_type",
    "trade_num",
    "volume",
    "turnover",
    "bid_weighted_avg_price",
    "total_bid_volume",
    "ask_weighted_avg_price",
    "total_ask_volume",
    "pre_close_price",
    "last_price",
    "open_price",
    "high_price",
    "low_price",
    "upper_limit_price",
    "lower_limit_price",
    "close_price",
    "bid_depth",
    "ask_depth",
    "bid_price0",
    "bid_price1",
    "bid_price2",
    "bid_price3",
    "bid_price4",
    "bid_price5",
    "bid_price6",
    "bid_price7",
    "bid_price8",
    "bid_price9",
    "ask_price0",
    "ask_price1",
    "ask_price2",
    "ask_price3",
    "ask_price4",
    "ask_price5",
    "ask_price6",
    "ask_price7",
    "ask_price8",
    "ask_price9",
    "bid_volume0",
    "bid_volume1",
    "bid_volume2",
    "bid_volume3",
    "bid_volume4",
    "bid_volume5",
    "bid_volume6",
    "bid_volume7",
    "bid_volume8",
    "bid_volume9",
    "ask_volume0",
    "ask_volume1",
    "ask_volume2",
    "ask_volume3",
    "ask_volume4",
    "ask_volume5",
    "ask_volume6",
    "ask_volume7",
    "ask_volume8",
    "ask_volume9",
    "trading_phase_code",
]

depth_column_names = [
    "data_time",
    "instrument_id",
    "exchange_id",
    "instrument_type",
    "price",
    "volume",
    "side",
]

tick_column_names = [
    "data_time",
    "instrument_id",
    "exchange_id",
    "instrument_type",
    "bid_price",
    "ask_price",
    "bid_volume",
    "ask_volume",
]


class PyStreamDataBatcher:
    def __init__(self, stream_data_batcher: wc.StreamDataBatcher):
        self.stream_data_batcher = stream_data_batcher

    def get_entrust_df(self, source, instrument_id, exchange_id):
        entrust_array = np.asarray(
            self.stream_data_batcher.get_entrust_buffer(source, instrument_id, exchange_id)
        )
        entrust_df = pd.DataFrame(entrust_array)
        entrust_df.columns = entrust_column_names
        return entrust_df

    def get_transaction_df(self, source, instrument_id, exchange_id):
        transaction_array = np.asarray(
            self.stream_data_batcher.get_transaction_buffer(source, instrument_id, exchange_id)
        )
        transaction_df = pd.DataFrame(transaction_array)
        transaction_df.columns = transaction_column_names
        return transaction_df

    def get_quote_df(self, source, instrument_id, exchange_id):
        quote_array = np.asarray(
            self.stream_data_batcher.get_quote_buffer(source, instrument_id, exchange_id)
        )
        quote_df = pd.DataFrame(quote_array)
        quote_df.columns = quote_column_names
        return quote_df

    def get_tree_df(self, source, instrument_id, exchange_id):
        tree_array = np.asarray(
            self.stream_data_batcher.get_tree_buffer(source, instrument_id, exchange_id)
        )
        tree_df = pd.DataFrame(tree_array)
        tree_df.columns = tree_column_names
        return tree_df

    def get_depth_df(self, source, instrument_id, exchange_id):
        depth_array = np.asarray(
            self.stream_data_batcher.get_depth_buffer(source, instrument_id, exchange_id)
        )
        depth_df = pd.DataFrame(depth_array)
        depth_df.columns = depth_column_names
        return depth_df

    def get_tick_df(self, source, instrument_id, exchange_id):
        tick_array = np.asarray(
            self.stream_data_batcher.get_tick_buffer(source, instrument_id, exchange_id)
        )
        tick_df = pd.DataFrame(tick_array)
        tick_df.columns = tick_column_names
        return tick_df

    def pop_batched_entrust_until(self, until_time, instrument_id, exchange_id):
        self.stream_data_batcher.pop_batched_entrust_until(
            until_time, instrument_id, exchange_id
        )

    def pop_batched_transaction_until(self, until_time, instrument_id, exchange_id):
        self.stream_data_batcher.pop_batched_transaction_until(
            until_time, instrument_id, exchange_id
        )

    def pop_batched_quote_until(self, until_time, instrument_id, exchange_id):
        self.stream_data_batcher.pop_batched_quote_until(
            until_time, instrument_id, exchange_id
        )

    def pop_batched_tree_until(self, until_time, instrument_id, exchange_id):
        self.stream_data_batcher.pop_batched_tree_until(
            until_time, instrument_id, exchange_id
        )

    def pop_batched_depth_until(self, until_time, instrument_id, exchange_id):
        self.stream_data_batcher.pop_batched_depth_until(
            until_time, instrument_id, exchange_id
        )

    def pop_batched_tick_until(self, until_time, instrument_id, exchange_id):
        self.stream_data_batcher.pop_batched_tick_until(
            until_time, instrument_id, exchange_id
        )
