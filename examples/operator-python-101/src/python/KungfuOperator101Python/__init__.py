from kungfu.wingchun.constants import *

source = "xtp"
exchange = Exchange.SSE


def pre_start(context):
    context.log.info("pre start")
    context.subscribe(source, ["600000"], exchange)


def on_quote(context, quote, location):
    context.log.info("on quote: {}".format(quote))
