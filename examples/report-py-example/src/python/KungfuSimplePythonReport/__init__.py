
import kungfu
import time
from typing import Text
from kungfu.yijinjing import time as kft
from kungfu.yijinjing import journal as kfj

from typing import Text

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing



def on_quote(ctx, quote: lf.types.Quote):
    # ctx.log.info("on quote={}, at={}".format(quote, kft.strftime(ctx.now())))
    keeper = ctx.bookkeeper
    books = keeper.get_books()
    pass

def on_order(ctx, order: lf.types.Order):
    ctx.log.info("on order={}, at={}".format(order, kft.strftime(ctx.now())))
    pass

def on_read_synthetic_data(ctx, synthetic_data: lf.types.SyntheticData):
    # ctx.log.info("on synthetic_data={}, at={}".format(synthetic_data, kft.strftime(ctx.now())))
    pass


def on_trade(ctx, trade: lf.types.Trade):
    ctx.log.info("on trade={}, at={}".format(trade, kft.strftime(ctx.now())))
    pass
    
def sumerize(ctx) -> Text:
    ctx.log.info("call post_stop")
    return "{}"
  