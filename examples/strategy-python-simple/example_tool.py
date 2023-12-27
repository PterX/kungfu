import kungfu
from kungfu.yijinjing import time as kft
from kungfu.yijinjing import journal as kfj

from typing import Text

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing


def run(category: lf.enums.category, group: Text, name: Text, start_time: int, end_time: int, locator: yjj.locator):
	writer = wc.CacheToolWriter(category, group, name, start_time, end_time, locator)
	for i in range(100):
		quote = lf.types.Quote()
		writer.write_at(start_time + i, start_time + i, 0, quote)