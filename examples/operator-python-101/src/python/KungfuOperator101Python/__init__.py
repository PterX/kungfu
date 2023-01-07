from kungfu.wingchun.constants import *

source = "sim"
exchange = Exchange.SSE


def pre_start(context):
    context.log.info("pre start")
    context.subscribe(source, ["600000"], exchange)
    context.subscribe_operator("operator-cpp-test1","operator-cpp-test1")


def on_quote(context, quote, location):
    context.log.info("on quote: {}".format(quote))
    pass

def on_time_key_value(context, tkv, location):
    context.log.info("on time key value: {}".format(tkv))

def on_operator_state_change(context, operator_state_update, location):
    context.log.info("on operator state change: {}".format(operator_state_update))



