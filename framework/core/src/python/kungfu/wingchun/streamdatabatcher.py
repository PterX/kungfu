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
    "trading_phase_code",
    "bid_price",
    "ask_price",
    "bid_volume",
    "ask_volume",
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
    "trading_phase_code",
    "bid_price",
    "ask_price",
    "bid_volume",
    "ask_volume",
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

    def merge_columns(self, row):
        return row.tolist()

    def get_entrust_df(self, source, instrument_id, exchange_id):
        entrust_array = np.asarray(
            self.stream_data_batcher.get_entrust_buffer(
                source, instrument_id, exchange_id
            )
        )
        entrust_df = pd.DataFrame(entrust_array)
        entrust_df.columns = entrust_column_names
        return entrust_df

    def get_transaction_df(self, source, instrument_id, exchange_id):
        transaction_array = np.asarray(
            self.stream_data_batcher.get_transaction_buffer(
                source, instrument_id, exchange_id
            )
        )
        transaction_df = pd.DataFrame(transaction_array)
        transaction_df.columns = transaction_column_names
        return transaction_df

    def get_quote_df(self, source, instrument_id, exchange_id):
        quote_array = np.asarray(
            self.stream_data_batcher.get_quote_buffer(
                source, instrument_id, exchange_id
            )
        )
        quote_df = pd.DataFrame(quote_array)

        for i in range(22, 62, 10):
            start_col = i
            end_col = i + 9
            new_col_name = f"merged_column_{(i-22)//10 + 1}"
            quote_df[new_col_name] = quote_df.iloc[:, start_col : end_col + 1].apply(
                merge_columns, axis=1
            )

        quote_df = quote_df.drop(quote_df.columns[22:62], axis=1)
        quote_df.columns = quote_column_names
        return quote_df

    def get_tree_df(self, source, instrument_id, exchange_id):
        tree_array = np.asarray(
            self.stream_data_batcher.get_tree_buffer(source, instrument_id, exchange_id)
        )
        tree_df = pd.DataFrame(tree_array)

        for i in range(21, 61, 10):
            start_col = i
            end_col = i + 9
            new_col_name = f"merged_column_{(i-21)//10 + 1}"
            tree_df[new_col_name] = tree_df.iloc[:, start_col : end_col + 1].apply(
                merge_columns, axis=1
            )

        tree_df = tree_df.drop(tree_df.columns[21:61], axis=1)
        tree_df.columns = tree_column_names
        return tree_df

    def get_depth_df(self, source, instrument_id, exchange_id):
        depth_array = np.asarray(
            self.stream_data_batcher.get_depth_buffer(
                source, instrument_id, exchange_id
            )
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
