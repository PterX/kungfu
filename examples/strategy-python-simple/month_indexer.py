import kungfu
from kungfu.yijinjing import time as kft
from kungfu.yijinjing import journal as kfj

from typing import Text

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing

def find_md_slice_location(ctx, nano_time, group, name, instrument_id, exchange_id, data_type):
	slice_end = get_md_slice_end_time(ctx, nano_time, group, name, instrument_id, exchange_id, data_type)
	dir_name = "{}_{}@{}".format(data_type, instrument_id, exchange_id)
	slice_locator = yjj.locator(lf.enums.mode.DATA, ["month_md", "until" + kft.strftime(slice_end, kft.SESSION_DATETIME_FORMAT), dir_name])
	slice_location = yjj.location(lf.enums.mode.DATA, lf.enums.category.MD, group, name, slice_locator)
	return slice_location

def get_md_slice_end_time(ctx, nano_time, group, name, instrument_id, exchange_id, data_type):
	return end_of_month(nano_time)

def find_operator_slice_location(ctx, nano_time, group, name):
	slice_end = get_operator_slice_end_time(ctx, nano_time, group, name)
	dir_name = "{}_{}".format(group, name)
	slice_locator = yjj.locator(lf.enums.mode.DATA, ["month_operator", "until" + kft.strftime(slice_end, kft.SESSION_DATETIME_FORMAT), dir_name])
	slice_location = yjj.location(lf.enums.mode.DATA, lf.enums.category.OPERATOR, group, name, slice_locator)
	return slice_location

def get_operator_slice_end_time(ctx, nano_time, group, name):
	return end_of_month(nano_time)

def end_of_month(nano_time):
	dt = kft.to_datetime(nano_time)
	next_month, relevant_year = (dt.month + 1, dt.year) if dt.month != 12 else (1, dt.year + 1)
	dt = dt.replace(year=relevant_year, month=next_month, day=1, hour=0, minute=0, second=0, microsecond=0)
	return kft.from_datetime(dt)

    