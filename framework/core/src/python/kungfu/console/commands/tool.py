#  SPDX-License-Identifier: Apache-2.0

import click
import importlib
import kungfu
import sys
import os

from kungfu.console.commands import kfc, PrioritizedCommandGroup
from kungfu.yijinjing import time as kft

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing


@kfc.command(help_priority=2)
@click.option("-b", "--begin", type=str, required=True, help="begin time")
@click.option("-e", "--end", type=str, required=True, help="begin time")
@click.option("-s", "--source", type=str, required=True, help="source")
@click.option("-t", "--tool_path", type=str, required=True, help="path to tool dynamic library")
@kfc.pass_context()
def tool(ctx, begin, end, source, tool_path):
    begin_time_stamp = kft.strptimes(begin)
    end_time_stamp = kft.strptimes(end)
    tool_dir = os.path.dirname(tool_path)
    name_no_ext = os.path.split(os.path.basename(tool_path))

    sys.path.append(os.path.relpath(tool_dir))

    module = importlib.import_module(name_no_ext[1].split('.')[0])
    tool_builder = getattr(module, "tool")
    tool = tool_builder(lf.enums.category.MD, source, begin_time_stamp, end_time_stamp, ctx.backtest_locator)
    tool.run()

