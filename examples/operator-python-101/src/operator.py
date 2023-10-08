import kungfu
import kungfu.yijinjing.time as kft
from kungfu.wingchun.constants import *
from kungfu.wingchun.utils import *
from datetime import datetime
import numpy as np
import pandas as pd
import json
lf=kungfu.__binding__.longfist
wc=kungfu.__binding__.wingchun
yjj=kungfu.__binding__.yijinjing


inst_df = pd.DataFrame()
basket_df = pd.DataFrame()

baskets = dict()

market_source = 'sim'

sb_commission = 0.02
pr_commission = 0.02



def pre_start(context):
    context.log.info("pre start")
    # context.subscribe_operator("bar", "my-bar")

def post_start(context):

    global basket_df
    global inst_df
    # 获取baskets
    baskets = context.basketorder_engine.baskets

    subscribe_instruments_map={}
    #初始化 basket_df
    for v in baskets.values():
        if v.type == BasketType.ETF and v.min_volume > 0:
            basket = dict()
            basket['id'] = int(v.id)
            basket['exchange_id'] = v.exchange_id
            basket['instrument_id'] = v.instrument_id
            basket['min_volume'] = int(v.min_volume)
            basket['cash_difference'] = v.cash_difference

            if v.exchange_id in subscribe_instruments_map.keys():
                subscribe_instruments_map[v.exchange_id].append(v.instrument_id)
            else:
                subscribe_instruments_map[v.exchange_id]=[v.instrument_id]
            basket_df = pd.concat([basket_df, pd.DataFrame([basket])], ignore_index=True)



    if not basket_df.empty:
        basket_df['inst_value'] = 0
        basket_df['inst_buy_commssion'] = 0
        basket_df['inst_sell_commssion'] = 0
        basket_df['last_price'] = 0
        basket_df['update_time'] = yjj.now_in_nano()
        basket_df['inst_update_time'] = yjj.now_in_nano()
        basket_df.set_index(['exchange_id', 'instrument_id'], inplace=True)
        basket_df = basket_df.sort_index()
    else:
        context.log.error("ETF is empty")
        return 

    

    # 获取 basket_instruments
    baskets_instruments = context.basketorder_engine.basket_instruments
    # 初始化 inst_df
    i=0
    for v in baskets_instruments.values():
        if v.basket_uid in basket_df['id'].values and  v.exchange_id != 'Unkown' and v.volume > 0:
            i+=1
            if i< 10000:
                context.log.info(f"i:{i}")
                inst = dict()
                inst['basket_uid'] = int(v.basket_uid)
                inst['instrument_id'] = v.instrument_id
                inst['exchange_id'] = v.exchange_id
                inst['volume'] = int(v.volume)
                inst['replace_flag'] = int(v.replace_flag)
                inst['cash_premium_ratio'] = v.cash_premium_ratio
                inst['replace_balance'] = v.replace_balance

                if v.exchange_id in subscribe_instruments_map.keys():
                    subscribe_instruments_map[v.exchange_id].append(v.instrument_id)
                else:
                    subscribe_instruments_map[v.exchange_id]=[v.instrument_id]
                

                inst_df = pd.concat([inst_df, pd.DataFrame([inst])], ignore_index=True)

    if not inst_df.empty:
        inst_df['update_time'] = yjj.now_in_nano()
        inst_df['last_price'] = 0
        inst_df.set_index(['exchange_id', 'instrument_id'], inplace=True)
        inst_df = inst_df.sort_index()
    else:
        context.log.error("ETF instrument is empty")

    

    #订阅行情
    for exchange, instruments_list in subscribe_instruments_map.items():
        context.log.info(f"exchange:{ exchange },instruments_list: {instruments_list}")
        context.subscribe(market_source,instruments_list,exchange)


    context.add_time_interval(3 * 10**(9), lambda ctx, event: calculte_iopv_and_publish(ctx))


def on_quote(context, quote, location, dest_id):
    global inst_df
    global basket_df
    context.log.info("on quote: {}".format(quote))
    index_tuple = (quote.exchange_id, quote.instrument_id)

    if index_tuple in inst_df.index:
        inst_df.loc[index_tuple, 'last_price'] = quote.last_price
        inst_df.loc[index_tuple,'update_time'] = yjj.now_in_nano()

    if index_tuple in basket_df.index:
        basket_df.loc[index_tuple, 'last_price'] = quote.last_price
        basket_df.loc[index_tuple,'update_time'] = yjj.now_in_nano()


# def on_synthetic_data(context, synthetic_data, location, dest_id):
#     context.log.info("on_synthetic_data: {}".format(synthetic_data))


# def on_operator_state_change(context, operator_state_update, location):
#     context.log.info("on operator state change: {}".format(operator_state_update))


def calculte_iopv_and_publish(context):
    global inst_df
    global basket_df

    if basket_df.empty or inst_df.empty:
        context.log.error("ETF or ETF instrument is empty")
        return

    inst_df['value'] = np.where((inst_df['replace_flag'] == 2) | (inst_df['replace_flag'] == 4) | (inst_df['replace_flag'] == 6), inst_df['replace_balance'], inst_df['last_price']* inst_df['volume'])
    inst_df['buy_commission'] = np.where(
        (inst_df['replace_flag'] == 2) | (inst_df['replace_flag'] == 4) | (inst_df['replace_flag'] == 6),
        0,
        inst_df.apply(lambda row: max(row['last_price'] * row['volume'] * sb_commission, 5) + row['last_price'] * row['volume'] * 0.00001, axis=1)
    )
    inst_df['sell_commission'] = np.where(
        (inst_df['replace_flag'] == 2) | (inst_df['replace_flag'] == 4) | (inst_df['replace_flag'] == 6),
        0,
        inst_df.apply(lambda row: max(row['last_price'] * row['volume'] * sb_commission, 5) + row['last_price'] * row['volume'] * 0.00101, axis=1)
    )

    
    #对value ,sell_commission, buy_commission进行group_by
    inst_value = inst_df.groupby('basket_uid').value.sum()
    inst_sell_commssion = inst_df.groupby('basket_uid').buy_commission.sum()
    inst_buy_commssion = inst_df.groupby('basket_uid').sell_commission.sum()
    inst_update_time = inst_df.groupby('basket_uid').update_time.min()


    basket_df['inst_value'] = basket_df['id'].map(inst_value).fillna(basket_df['inst_value'])
    basket_df['inst_sell_commssion'] = basket_df['id'].map(inst_sell_commssion).fillna(basket_df['inst_sell_commssion'])
    basket_df['inst_buy_commssion'] = basket_df['id'].map(inst_buy_commssion).fillna(basket_df['inst_buy_commssion'])
    basket_df['inst_update_time'] = basket_df['id'].map(inst_update_time).fillna(basket_df['inst_update_time'])


    basket_df['pr_commmission'] = basket_df['last_price'] * basket_df['min_volume'] * pr_commission
    basket_df['buy_commission'] = basket_df.apply(lambda row: max(row['last_price'] * row['min_volume'] * sb_commission, 5) + row['last_price'] * row['min_volume'] * 0.00001, axis=1)
    basket_df['sell_commission'] = basket_df.apply(lambda row: max(row['last_price'] * row['min_volume'] * sb_commission, 5) + row['last_price'] * row['min_volume'] * 0.00101, axis=1)
    

    basket_df['iopv'] = (basket_df['inst_value'] + basket_df['cash_difference'])/basket_df['min_volume']
    basket_df['discount'] = basket_df['inst_value'] + basket_df['cash_difference'] - basket_df['last_price']* basket_df['min_volume'] - basket_df['pr_commmission'] - basket_df['buy_commission'] - basket_df['inst_sell_commssion']
    basket_df['premium'] = basket_df['last_price'] * basket_df['min_volume']- (basket_df['inst_value'] + basket_df['cash_difference'])  - basket_df['pr_commmission'] - basket_df['sell_commission'] - basket_df['inst_buy_commssion']



    for index, row in basket_df.iterrows():
    # 对last_price 不为0 的basket逐个进行发布
         if row['last_price'] > 0:
            sy_data = row.loc[['last_price','iopv','discount','premium','inst_update_time']]
            context.publish_synthetic_data(str(int(row['id'])), json.dumps(sy_data.to_dict()))



    