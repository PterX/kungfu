#  SPDX-License-Identifier: Apache-2.0

import datetime


class Calendar:
    def __init__(self, ctx):
        self.ctx = ctx
        self.update_trading_day(datetime.datetime.now())

    def update_trading_day(self, now):
        self.date = now.date()
        if now.hour >= 18:
            self.date = self.date + datetime.timedelta(days=1)
        while not self.is_trading_day(self.date):
            self.date = self.date + datetime.timedelta(days=1)

    def is_trading_day(self, dt):
        return dt.isoweekday() <= 5
