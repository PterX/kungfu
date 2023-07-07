
import kungfu
from kungfu.yijinjing import time as kft
from kungfu.yijinjing import journal as kfj

from typing import Text

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing


def run(slice_tool: wc.SliceTool):
	for i in range(100):
		quote = lf.types.Quote()
		slice_tool.write_at(slice_tool.begin_time + i, slice_tool.begin_time + i, 0, quote)

