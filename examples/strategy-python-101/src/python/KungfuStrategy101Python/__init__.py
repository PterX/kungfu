import random
from kungfu.wingchun.constants import *
import kungfu

wc = kungfu.__binding__.wingchun

source = "sim"
md_source = "xtp"
exchange = Exchange.SSE


def pre_start(context):
    context.log.info(
        f"is_book_held: {context.is_book_held()}, is_positions_held: {context.is_positions_held()}"
    )
    context.log.info("pre start")
    context.add_account(source, "123456")
    context.subscribe(source, ["600000"], exchange)
    context.subscribe(md_source, ["600004"], exchange)
    context.subscribe(md_source, ["600009"], exchange)


def post_start(context):
    account_uid = context.get_account_uid(source, "123456")
    context.log.info(f"account {source} '123456', account_uid: {account_uid}")

    instrument_factor_key = wc.utils.hash_instrument(account_uid, exchange, "600000")
    instrument_key = wc.utils.hash_instrument(exchange, "600000")
    context.log.info(
        f"instrument_factor_key: {instrument_factor_key}, instrument_key: {instrument_key}"
    )

    if instrument_factor_key in context.book.instrument_factors:
        instrument_factor = context.book.instrument_factors[instrument_factor_key]
        context.log.info(f"instrument_factor {instrument_factor}")

    if instrument_key in context.book.instruments:
        instrument = context.book.instruments[instrument_key]
        context.log.info(f"instrument {instrument}")


def on_quote(context, quote, location, dest):
    side = random.choice([Side.Buy, Side.Sell])
    side = Side.Buy
    price = quote.ask_price[0] if side == Side.Buy else quote.bid_price[0]
    price_type = random.choice([PriceType.Any, PriceType.Limit])
    context.insert_order(
        quote.instrument_id, exchange, source, "123456", price, 100, price_type, side
    )

    instrument_key = wc.utils.hash_instrument(quote.exchange_id, quote.instrument_id)
    if instrument_key in context.book.instruments:
        context.log.info(
            f"{quote.exchange_id}, {quote.instrument_id}, {instrument_key}, {context.book.instruments[instrument_key]}"
        )


def on_synthetic_data(context, synthetic_dataa, location, dest):
    context.log.info("on_synthetic_data: {}".format(synthetic_dataa))


def on_order(context, order, location, dest):
    pass


def on_trade(context, trade, location, dest):
    pass


def on_register(context, location):
    for key in context.get_account_book(source, "123456").instruments:
        context.log.info(f"{key}, {instrument}")
