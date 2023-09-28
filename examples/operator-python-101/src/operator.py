from kungfu.wingchun.constants import *
import pandas as pd
import numpy as np

source = "sim"
exchange = Exchange.SSE

inst_df = pd.DataFrame()
basket_df = pd.DataFrame()

baskets = dict()

market_source = 'sim'

sb_commission = 0.02
pr_commission = 0.02



def pre_start(context):
    context.log.info("pre start")
    context.subscribe(source, ["600000"], exchange)
    # context.subscribe_operator("bar", "my-bar")

def post_start(context):

    global basket_df
    global inst_df
    # 获取baskets
    baskets = context.book.baskets

    #初始化 basket_df
    for v in baskets.values():
        if v.type == BasketType.ETF:
            basket = dict()
            basket['id'] = v.id
            basket['exchange_id'] = v.exchange_id
            basket['instrument_id'] = v.instrument_id
            basket['min_volume'] = v.min_volume
            basket['cash_difference'] = v.cash_difference

            basket_df = pd.concat([basket_df, pd.DataFrame([basket])], ignore_index=True)

    basket_df.set_index(['exchange_id', 'instrument_id'], inplace=True)


    

    # 获取 basket_instruments
    baskets_instruments = context.book.basket_instruments
    # 初始化 inst_df
    for v in baskets_instruments.values():
        inst = dict()
        inst['basket_uid'] = v.basket_uid
        inst['instrument_id'] = v.instrument_id
        inst['exchange_id'] = v.exchange_id
        inst['volume'] = v.volume
        inst['replace_flag'] = v.replace_flag
        inst['cash_premium_ratio'] = v.cash_premium_ratio
        inst['replace_balance'] = v.replace_balance

        inst_df = pd.concat([inst_df, pd.DataFrame([inst])], ignore_index=True)

    inst_df.set_index(['exchange_id', 'instrument_id'], inplace=True)


    #对行情进行订阅:
    context.subsceibe(market_source, instrument_list, exchange)

def on_quote(context, quote, location, dest_id):
    global inst_df
    context.log.info("on quote: {}".format(quote))
    inst_df.loc[(quote.exchange_id, quote.instrument_id),'last_price'] = quote.last_price
    basket_df.loc[(quote.exchange_id, quote.instrument_id),'last_price'] = quote.last_price


def on_synthetic_data(context, synthetic_dataa, location, dest_id):
    context.log.info("on_synthetic_data: {}".format(synthetic_dataa))


def on_operator_state_change(context, operator_state_update, location):
    context.log.info("on operator state change: {}".format(operator_state_update))


def calculte_iopv_and_publish(context):
    global inst_df
    global basket_df

    inst_df['value'] = np.where(inst_df['replace_flag']== 2 or inst_df['replace_flag']== 4 or inst_df['replace_flag']== 6, inst_df['replace_balance'], inst_df['last_price']* inst_df['volume'])
    inst_df['buy_commission'] = np.where(inst_df['replace_flag']== 2 or inst_df['replace_flag']== 4 or inst_df['replace_flag']== 6,0,max(inst_df['last_price'] * inst_df['volume'] * sb_commission,5) + inst_df['last_price'] * inst_df['volume'] * 0.00001)
    inst_df['sell_commission'] = np.where(inst_df['replace_flag']== 2 or inst_df['replace_flag']== 4 or inst_df['replace_flag']== 6,0,max(inst_df['last_price'] * inst_df['volume'] * sb_commission,5) + inst_df['last_price'] * inst_df['volume'] * 0.00101)
    
    #对value ,sell_commission, buy_commission进行group_by
    basket_df['inst_value']
    basket_df['inst_sell_commssion']
    basket_df['inst_buy_commssion']

    basket_df['pr_commmission'] = basket_df['last_price'] * basket_df['min_volume'] * pr_commission
    basket_df['buy_commission'] = max(basket_df['last_price'] * basket_df['min_volume'] * sb_commission, 5) + basket_df['last_price'] * basket_df['volume'] * 0.00001
    basket_df['sell_commission'] = max(basket_df['last_price'] * basket_df['min_volume'] * sb_commission, 5) + basket_df['last_price'] * basket_df['volume'] * 0.00001

    basket_df['iopv'] = (basket_df['inst_value'] + basket_df['cash_difference'])/basket_df['min_volume']
    basket_df['discount'] = basket_df['inst_value'] + basket_df['cash_difference'] - basket_df['last_price']* basket_df['min_volume'] - basket_df['pr_commmission'] - basket_df['buy_commission'] - basket_df['inst_sell_commssion']
    basket_df['discount'] = basket_df['last_price'] * basket_df['min_volume']- (basket_df['inst_value'] + basket_df['cash_difference'])  - basket_df['pr_commmission'] - basket_df['sell_commission'] - basket_df['inst_buy_commssion']

    # 对last_price 不为0 的basket逐个进行发布

    