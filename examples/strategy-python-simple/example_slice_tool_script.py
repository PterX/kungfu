
import kungfu
import time
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


	# location = slice_tool.find_md_slice_location(slice_tool.begin_time, "", "", 401)
	# slice_tool.join(location, 0, slice_tool.begin_time)
	# counter = 0
	# while slice_tool.data_available():
	# 	frame = slice_tool.current_frame() 	
	# 	print(frame.gen_time)
	# 	slice_tool.next()
	# 	counter += 1
	
     
