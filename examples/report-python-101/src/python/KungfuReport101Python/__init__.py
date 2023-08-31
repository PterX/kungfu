
import kungfu
from datetime import datetime, timedelta
import pandas as pd
import numpy as np
import time
import json
from typing import Text
from kungfu.yijinjing import time as kft
from kungfu.yijinjing import journal as kfj
from kungfu.wingchun.constants import *
from pprint import pprint
from collections import defaultdict

from typing import Text

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing


class BalanceResult:
    def __init__(self, ctx, initial_capital: float,  begin_time: int, risk_free_rate=0.05) -> None:
        self.ctx = ctx
        self.capital =  initial_capital
        self.last_now = begin_time
        self.risk_free = risk_free_rate
        self.dates = [self.begin_period(kft.to_datetime(begin_time))]
        self.balances = [initial_capital]
        
    def n_period_cross(self, nano_now: int) -> int:
        last_datetime = kft.to_datetime(self.last_now)
        now_datetime = kft.to_datetime(nano_now)

        n_period = 0
        next_datetime = self.next_period(last_datetime)
        while next_datetime < now_datetime:
            n_period += 1
            self.dates.append(next_datetime)
            next_datetime = self.next_period(next_datetime)
        self.last_now = nano_now
        return n_period

    @classmethod
    def n_period_between(cls, begin: datetime, end:  datetime) -> int:
        n_period = 0
        next_datetime = cls.next_period(begin)
        while next_datetime < end:
            n_period += 1
            next_datetime = cls.next_period(next_datetime)
        return n_period

    @staticmethod
    def begin_period(date : datetime) -> datetime:
        date = date.replace(minute=0, second=0, microsecond=0)
        return date

    @staticmethod
    def period_delta() -> timedelta:
        return timedelta(hours=1)
    
    @staticmethod
    def is_trading_time(date : datetime) -> bool:
        return True

    @classmethod
    def next_period(cls, date : datetime) -> datetime:
        date = cls.begin_period(date)
        date += cls.period_delta()
        while not cls.is_trading_time(date):
            date += cls.period_delta()
        return date

    @classmethod
    def annual_periods(cls, begin: datetime):
        year_begin = begin.replace(month=1, day=1, hour=0, minute=0, second=0, microsecond=0)
        year_end = year_begin.replace(year=year_begin.year+1)
        return cls.n_period_between(year_begin, year_end)
    

    def get_strategy_asset(self, bookkeeper: wc.Bookkeeper) -> lf.types.Asset:
        books = bookkeeper.get_books()
        for book in books.values():
            asset = book.asset
            if asset.ledger_category == LedgerCategory.Strategy:
                return asset
        return None 

    def update(self, nano_now: int, bookkeeper: wc.Bookkeeper):
        n_period = self.n_period_cross(nano_now)
        if n_period == 0:
            return

        asset = self.get_strategy_asset(bookkeeper)
        balance = self.capital + asset.realized_pnl + asset.unrealized_pnl - asset.accumulated_fee
        for index in reversed(range(n_period)):
            if index == 0:
                self.balances.append(balance)
            else:
                self.balances.append(self.balances[-1])
        assert(len(self.dates) == len(self.balances))
    
   
    def calculate_statistrics(self):
        df = pd.DataFrame({'date': self.dates, 'balance': self.balances}).set_index("date")
        if (df["balance"] < 0).any():
            self.ctx.log.error("balance can not be less than zero, otherwise some statistic may wrongly calculated. raise up the your initial capital")
        df["return"] = np.log(df["balance"] / df["balance"].shift(1)).fillna(0)
        df["highlevel"] = df["balance"].rolling(
                    min_periods=1, window=len(df), center=False).max()
        df["drawdown"] = df["balance"] - df["highlevel"]
            
        df["ddpercent"] = df["drawdown"] / df["highlevel"] * 100
        df["principal_ddpercent"] = df["drawdown"] / df["balance"][0] * 100

        annual_periods = self.annual_periods(self.dates[0])

        start_date = df.index[0]
        end_date = df.index[-1]
        total_periods = len(df)
        profit_periods = len(df[df["drawdown"] == 0])
        loss_periods = len(df[df["drawdown"] < 0])

        end_balance = df["balance"].iloc[-1]
        max_drawdown = df["drawdown"].min()
        highlevel_idx = pd.Series(np.append(np.where(df["drawdown"] == 0)[0], len(df)))
        longest_drawdown_duration = (highlevel_idx - highlevel_idx.shift(1)).fillna(0).max()
        max_ddpercent = df["ddpercent"].min()
        principal_max_ddpercent = df["principal_ddpercent"].min()
        # max_drawdown_end = df["drawdown"].idxmin()
        # max_drawdown_begin = df["balance"][:max_drawdown_end].idxmax()
        # max_drawdown_duration_periods = self.n_period_between(max_drawdown_begin, max_drawdown_end )

        total_return = (end_balance / self.capital - 1) * 100
        annual_return = total_return / total_periods * annual_periods
        period_return = df["return"].mean() * 100
        return_std = df["return"].std() * 100

        if return_std:
            period_risk_free = self.risk_free / np.sqrt(annual_periods)
            sharpe_ratio = (period_return - period_risk_free) / return_std * np.sqrt(annual_periods)

        else:
            sharpe_ratio = 0
        
        return_drawdown_ratio = (end_balance - self.capital) / - max_drawdown

        statistics = {
            "start_date": str(start_date),
            "end_date": str(end_date),
            "total_periods": total_periods,
            "profit_periods": profit_periods,
            "loss_periods": loss_periods,
            "capital": self.capital,
            "end_balance": end_balance,
            "max_drawdown": max_drawdown,
            "max_ddpercent": max_ddpercent,
            "principal_max_ddpercent": principal_max_ddpercent,
            # "max_drawdown_duration_periods": max_drawdown_duration_periods,
            "longest_drawdown_duration": longest_drawdown_duration,
            "total_return": total_return,
            "annual_return": annual_return,
            "period_return": period_return,
            "return_std": return_std,
            "sharpe_ratio": sharpe_ratio,
            "return_drawdown_ratio": return_drawdown_ratio,
        }

        for key, value in statistics.items():
            if value in (np.inf, -np.inf):
                value = 0
                statistics[key] = value
            if isinstance(value, np.number):
                statistics[key] = float(value)
        return statistics



        
balance_result = None

def init(ctx):
    global balance_result
    balance_result = BalanceResult(ctx, initial_capital=100000000, begin_time=ctx.now(), risk_free_rate=0.05)

def on_quote(ctx, quote: lf.types.Quote):
    # ctx.log.info("on quote={}, at={}".format(quote, kft.strftime(ctx.now())))
    pass

def on_order(ctx, order: lf.types.Order):
    # ctx.log.info("on order={}, at={}".format(order, kft.strftime(ctx.now())))
    pass

def on_read_synthetic_data(ctx, synthetic_data: lf.types.SyntheticData):
    # ctx.log.info("on synthetic_data={}, at={}".format(synthetic_data, kft.strftime(ctx.now())))
    pass

def on_trade(ctx, trade: lf.types.Trade):
    balance_result.update(ctx.now(), ctx.bookkeeper)
    # ctx.log.info("on trade={}, at={}".format(trade, kft.strftime(ctx.now())))

    books = ctx.bookkeeper.get_books()
    combined_asset = defaultdict(list)
    for asset in map(lambda book: json.loads(str(book.asset)), books.values()):
        for key, value in asset.items():
            combined_asset[key].append(value)
    for book in books.values():
        long_positions = list(book.long_positions.values())
        short_positions = list(book.short_positions.values())
    # print(list(book.trades.items()))
    # print("long_positions:")
    # print(list(long_positions.items()))
    # print("short_positions:")
    # print(list(short_positions.items()))
    # pprint(combined_asset)
    pass
    
def sumerize(ctx) -> Text:
    balance_result.update(ctx.now(), ctx.bookkeeper)
    statistics = balance_result.calculate_statistrics()

    term_sheet = {
            "start_date": "交易开始时间",
            "end_date":"交易终止时间",
            "total_periods": "总交易周期数",
            "profit_periods": "盈利周期数",
            "loss_periods": "亏损周期数",
            "capital": "初始资金",
            "end_balance": "终止资金",
            "max_drawdown": "最大回撤",
            "max_ddpercent": "最大回撤百分比",
            "principal_max_ddpercent": "相对本金最大回撤百分比",
            # "max_drawdown_duration_periods":,
            "longest_drawdown_duration": "最长回撤周期数",
            "total_return":  "总收益率",
            "annual_return": "年化收益率",
            "period_return": "周期平均收益率",
            "return_std": "收益率标准差",
            "sharpe_ratio": "夏普率",
            "return_drawdown_ratio": "收益回撤比",
    }
    # for key, value in statistics.items():
    #     ctx.log.info("{}={}".format(term_sheet[key], value))

    return json.dumps(statistics, indent=4)
  