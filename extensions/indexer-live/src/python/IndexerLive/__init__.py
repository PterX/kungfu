import kungfu
from kungfu.yijinjing import time as kft
from kungfu.yijinjing import journal as kfj

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing


def find_md_slice_location(
    ctx, nano_time, group, name, instrument_id, exchange_id, data_type
):
    slice_location = yjj.location(
        lf.enums.mode.LIVE, lf.enums.category.MD, group, name, ctx.runtime_locator
    )
    return slice_location


def get_md_slice_end_time(
    ctx, nano_time, group, name, instrument_id, exchange_id, data_type
):
    return kft.today_start() + kft.DAY_IN_NANO


def find_operator_slice_location(ctx, nano_time, group, name):
    slice_location = yjj.location(
        lf.enums.mode.LIVE, lf.enums.category.OPERATOR, group, name, ctx.runtime_locator
    )
    return slice_location


def get_operator_slice_end_time(ctx, nano_time, group, name):
    return kft.today_start() + kft.DAY_IN_NANO
