#  SPDX-License-Identifier: Apache-2.0

import click
import importlib
import kungfu
from pathlib import Path
import sys
import os

from kungfu.console.commands import kfc
from kungfu.yijinjing import time as kft
from kungfu.yijinjing.log import find_logger
from kungfu.yijinjing import journal as kfj


lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing


@kfc.command(help_priority=2)
@click.option("-b", "--begin", type=str, required=True, help="begin time")
@click.option("-e", "--end", type=str, required=True, help="end time")
@click.option(
    "-c",
    "--category",
    type=click.Choice(kfj.CATEGORIES.keys()),
    help="category",
)
@click.option("-g", "--group", type=str, required=True, help="source")
@click.option("-n", "--name", type=str, required=True, help="source")
@click.option(
    "-t", "--tool_path", type=str, required=True, help="path to tool dynamic library"
)
@kfc.pass_context()
def tool(ctx, begin, end, category, group, name, tool_path):
    location = yjj.location(
        kfj.MODES["backtest"],
        kfj.CATEGORIES[category],
        group,
        name,
        ctx.backtest_locator,
    )
    logger = find_logger(location, ctx.log_level)
    begin_time_stamp = kft.strptimes(begin)
    end_time_stamp = kft.strptimes(end)
    tool_path = Path(tool_path)
    tool_dir = str(tool_path.parent)
    sys.path.append(os.path.relpath(tool_dir))

    module_name = tool_path.stem.split(".")[0]
    logger.debug(f"loading module from {tool_path}")
    module = importlib.import_module(module_name)

    if not tool_path.suffix.endswith("py"):
        tool_builder = getattr(module, "tool")
        tool = tool_builder(
            kfj.CATEGORIES[category],
            group,
            name,
            begin_time_stamp,
            end_time_stamp,
            ctx.backtest_locator,
        )
        tool.run()
    else:
        tool_func = getattr(module, "run")
        tool_func(
            kfj.CATEGORIES[category],
            group,
            name,
            begin_time_stamp,
            end_time_stamp,
            ctx.backtest_locator,
        )
