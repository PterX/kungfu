# -*- coding: UTF-8 -*-
import kungfu.yijinjing.time as kft
from kungfu.wingchun.constants import *
from pykungfu import wingchun as wc
from collections import deque
from numpy import nan
import datetime
import numpy as np
import csv
import random
import json

# source = "heiyi"
source = "heiyi"
account = "123456"
exchange = Exchange.SHFE
tickers = ["CF202005"]
volume = 1

# file = open("/cpfs/fs3200/home/cta/heiyi-sample/strategy-heiyi/output/trade_test.csv", mode='w')
# # file = open("/home/wei/Desktop/kf/kungfu-2.5/examples/strategy-python-backtest/trade.csv", mode='w')
# writer = csv.writer(file)


# 启动前回调，添加交易账户，订阅行情，策略初始化计算等
def pre_start(context):
    print("!!!!!!!!!!!!!!!", context.arguments)
    context.log.info("preparing strategy")
    context.add_account(source, account)
    context.subscribe(source, tickers, exchange)
    # context.subscribe_all(source)
    context.data = {}
    context.last_minute = {}
    context.last_data = {}
    context.last_minute_data = {}
    context.order_status = {}
    context.long_positions = {}
    context.short_positions = {}
    context.trade_time = {}
    context.prevday = {}
    context.today = {}
    context.max_trade_time = 110
    for ticker in tickers:
        context.last_minute[ticker] = None
        context.last_data[ticker] = None
        context.last_minute_data[ticker] = None
        context.long_positions[ticker] = {"avg_open_price": 0.0, "volume": 0}
        context.short_positions[ticker] = {"avg_open_price": 0.0, "volume": 0}
        context.prevday[ticker] = None
        context.today[ticker] = None
        context.trade_time[ticker] = 0


def cb(ctx, e):
    print("timer")
    pass


def on_quote(context, quote, location, dest):
    context.add_timer(context.now(), cb)
    # context.log.info("Quote time: {}".format(datetime.datetime.fromtimestamp(quote.data_time/1e9)))
    # quote = eval(str(quote))
    quote = json.loads(str(quote))
    context.log.info("quote, {}".format(quote))
    ticker = quote["instrument_id"]
    current_timestamp = datetime.datetime.fromtimestamp(quote["data_time"] / 1e9)
    current_minute = current_timestamp.strftime("%Y/%m/%d %H:%M")
    # context.today[ticker] = current_timestamp.strftime("%Y%m%d")
    # if (context.prevday[ticker] != context.today[ticker]) & (
    #     context.prevday[ticker] is not None
    # ):
    #     pass
    #     context.trade_time[ticker] = 0
    #     context.prevday[ticker] = context.today[ticker]
    # elif (context.prevday[ticker] != context.today[ticker]) & (
    #     context.prevday[ticker] is None
    # ):
    #     context.prevday[ticker] = context.today[ticker]

    # # context.log.info("ticker: {}".format(ticker))
    # # context.log.info("current_timestamp: {}".format(current_timestamp))
    # # context.log.info("current_minute: {}".format(current_minute))
    # # context.log.info("last_minute: {}".format(context.last_minute))

    # data = None
    # new_quote = quote.copy()
    # if (current_minute != context.last_minute[ticker]) & (
    #     context.last_minute[ticker] is not None
    # ):
    #     context.last_data[ticker]["data_time"] = context.last_minute[ticker]
    #     if context.last_data[ticker]["last_price"] > 1e20:
    #         context.last_data[ticker]["last_price"] = np.nan
    #     context.last_minute[ticker] = current_minute
    #     context.last_minute_data[ticker] = context.last_data[ticker].copy()
    #     context.log.info(
    #         "last_minute_data: {}".format(context.last_minute_data[ticker])
    #     )
    #     data = context.last_data[ticker].copy()
    # elif (current_minute != context.last_minute[ticker]) & (
    #     context.last_minute[ticker] is None
    # ):
    #     context.last_minute[ticker] = current_minute
    # else:
    #     pass
    # context.last_data[ticker] = new_quote.copy()

    # if (data is not None) & (context.trade_time[ticker] <= context.max_trade_time):
    #     context.log.info("Trade time: {}".format(context.trade_time[ticker]))
    #     side = Side.Buy
    #     price = quote["ask_price"][0] + 1
    #     price_type = random.choice([PriceType.Any, PriceType.Limit])
    #     context.insert_order(
    #         quote["instrument_id"],
    #         quote["exchange_id"],
    #         source,
    #         account,
    #         price,
    #         1,
    #         price_type,
    #         side,
    #         Offset.Open,
    #     )
    #     context.trade_time[ticker] += 1


# 收到订单状态回报时回调
def on_order(context, order, location, dest):
    ticker = order.instrument_id
    order_id = order.order_id
    context.log.info("[order_info] {}".format(order))
    if order.status in [OrderStatus.Cancelled, OrderStatus.Filled]:
        context.log.info("Order_status is {}".format(order.status))
        context.order_status[ticker] = order.status


# 收到成交信息回报时回调
def on_trade(context, trade, location, dest):
    context.log.info(
        "[on_trade] id: {} , status: {}".format(trade.order_id, trade.volume)
    )
    writer.writerow(
        [
            trade.side,
            trade.offset,
            trade.price,
            trade.volume,
            trade.instrument_id,
            trade.exchange_id,
            trade.trade_time,
        ]
    )


def post_stop(context):
    pass
    # file.close()


def get_pos(context, ticker):
    long_pos = context.long_positions[ticker]
    short_pos = context.short_positions[ticker]
    return long_pos, short_pos


# def get_pos(context, data):
#     book = context.get_account_book(source, account)
#     long_pos = book.get_long_position(data['exchange_id'], data['instrument_id'])
#     short_pos = book.get_short_position(data['exchange_id'], data['instrument_id'])
#     return long_pos, short_pos

# # 撤单函数
# def cancel_order(context, ticker, order_id):
#     # 撤单
#     if context.order_status[ticker] not in [OrderStatus.Filled]:
#         action_id = context.cancel_order(order_id)
#         if action_id > 0:
#             context.log.info("[cancel order] (action_id){} (order_id){} ".format(action_id, order_id))
#     else:
#         context.log.info("Order is already filled")


# import random
# import csv
# from kungfu.wingchun.constants import *

# source = "heiyi"
# account = "123456"
# exchange = Exchange.SSE

# # file = open("/path/to/your/trade.csv", mode='w')
# file = open(
#     "/home/wei/Desktop/kf/kungfu-2.5/examples/strategy-python-backtest/trade.csv",
#     mode="w",
# )
# writer = csv.writer(file)


# def pre_start(context):

#     context.log.info("preparing strategy")

#     context.add_account(source, account)
#     context.subscribe(source, ["sc"], Exchange.SHFE)


# def on_quote(context, quote, location):
#     context.log.info(
#         "quote: bid/asks = {}/{}, datatime={}".format(
#             quote.bid_price[0], quote.ask_price[0], quote.data_time
#         )
#     )
#     side = random.choice([Side.Buy, Side.Sell])
#     side = Side.Buy
#     price = quote.ask_price[0] if side == Side.Buy else quote.bid_price[0]
#     price_type = random.choice([PriceType.Any, PriceType.Limit])
#     context.insert_order(
#         quote.instrument_id,
#         quote.exchange_id,
#         source,
#         account,
#         quote.bid_price[0] + 5,
#         100,
#         PriceType.Limit,
#         Side.Buy,
#         Offset.Open,
#     )


# def post_stop(context):
#     file.close()


# def on_order(context, order, location):
#     # context.log.info("order: {}".format(order))
#     pass


# def on_trade(context, trade, location):
#     context.log.info("trade: {}".format(trade))
#     writer.writerow(
#         [
#             trade.side,
#             trade.offset,
#             trade.price,
#             trade.volume,
#             trade.instrument_id,
#             trade.exchange_id,
#             trade.trade_time,
#         ]
#     )
