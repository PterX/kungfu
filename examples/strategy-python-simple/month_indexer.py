import kungfu
from kungfu.yijinjing import time as kft
from kungfu.yijinjing import journal as kfj
from kungfu.wingchun.constants import *
from kungfu.wingchun import msg

from datetime import datetime, timedelta

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing

def get_expire(instrument_id):
	contract = ''.join(filter(str.isdigit, instrument_id))
	if len(contract) == 6:
		contract_year = int(contract[:4])
	elif len(contract) == 3:
		contract_year = int("202" + contract[:1])
	else:
		contract_year = int("20" + contract[:2])
	contract_month = int(contract[-2:])
	next_month_dt = datetime(contract_year, contract_month, 1) + timedelta(days=32)
	last_day_of_month = next_month_dt - timedelta(days=next_month_dt.day)
	return kft.from_datetime(last_day_of_month)


def find_md_slice_location(ctx, nano_time, group, name, instrument_id, exchange_id, data_type):
	instrument_type = wc.utils.get_instrument_type(exchange_id, instrument_id)
	if instrument_type == InstrumentType.Stock or instrument_type == InstrumentType.Bond:
		if data_type not in (msg.Quote, msg.Entrust, msg.Transaction):
			return None
	elif instrument_type == InstrumentType.Future:
		if data_type not in (msg.Quote, ):
			return None
		expire_datetime = get_expire(instrument_id)
		if nano_time > expire_datetime:
			return None
		# TODO: future contrtact open_datetime

	# elif instrument_type == InstrumentType.Crypto
	# elif instrument_type == InstrumentType.CryptoFuture
	# elif instrument_type == InstrumentType.CryptoUFuture


		
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

	