from kungfu.wingchun.constants import *

source = "sim"
exchange = Exchange.SSE


def pre_start(context):
    context.log.info("pre start")
    context.log.info(f"operator_dir: {context.operator_dir}")
    context.subscribe(source, ["600000"], exchange)
    context.subscribe_operator("bar", "my-bar")


def on_quote(context, quote, location, dest):
    context.log.info("on quote: {}".format(quote))
    pass


def on_synthetic_data(context, synthetic_data, location, dest):
    context.log.info("on_synthetic_data: {}".format(synthetic_data))


def on_operator_state_change(context, operator_state_update, location):
    context.log.info("on operator state change: {}".format(operator_state_update))
