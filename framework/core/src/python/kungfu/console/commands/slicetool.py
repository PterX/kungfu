#  SPDX-License-Identifier: Apache-2.0

import click
import importlib
import kungfu
from pathlib import Path
import sys
import os

from kungfu.console.commands import kfc
from kungfu.console import site
from kungfu.yijinjing import time as kft
from kungfu.yijinjing.log import find_logger
from kungfu.yijinjing import journal as kfj
# tracing-foundation Phase 1: wingchun 交易运行时已 carve;sliceindexer 降级为 lazy 占位,
# 使 slicetool 命令可被注册表导入(仅在真正执行 slicetool 时才用到,Phase 1 不走)。
try:
    from kungfu.wingchun import sliceindexer
except (ImportError, AttributeError):
    sliceindexer = None


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
    "-t",
    "--tool_path",
    type=str,
    required=True,
    help="path to tool dynamic library or py script",
)
@click.option(
    "-i",
    "--indexer_path",
    type=str,
    required=False,
    help="path to indexer dynamic library",
)
@click.option(
    "-a",
    "--arguments",
    type=str,
    default="{}",
    required=False,
    help="arguments passed to SliceTool::get_arguments",
)
@click.option(
    "-o",
    "--overwrite",
    # type=click.Choice([False, True]),
    type=bool,
    default=True,
    required=False,
    help="do not use it until you really understand what you are doing!",
)
@click.option(
    "-s",
    "--size",
    type=int,
    default=32,
    required=False,
    help="journal size in MB",
)
@kfc.pass_context()
def slicetool(
    ctx,
    begin,
    end,
    category,
    group,
    name,
    tool_path,
    indexer_path,
    arguments,
    overwrite,
    size,
):
    location = yjj.location(
        kfj.MODES["data"],
        kfj.CATEGORIES[category],
        group,
        name,
        ctx.backtest_locator,
    )
    logger = find_logger(location, ctx.log_level)
    ctx.logger = logger
    begin_time_stamp = kft.strptimes(begin)
    end_time_stamp = kft.strptimes(end)
    tool_path = Path(tool_path)
    tool_dir = str(tool_path.parent)
    sys.path.append(os.path.relpath(tool_dir))
    site.setup(tool_dir)

    module_name = tool_path.stem.split(".")[0]
    logger.debug(f"loading module from {tool_path}")
    module = importlib.import_module(module_name)

    if indexer_path:
        indexer = sliceindexer.SliceIndexer(
            ctx, begin_time_stamp, end_time_stamp, indexer_path
        )
    else:
        # indexer = wc.DayIndexer(begin_time_stamp, end_time_stamp)
        indexer = wc.SliceIndexer(begin_time_stamp, end_time_stamp)
    ctx.indexer = indexer
    try:
        slice_tool_builder = getattr(module, "slice_tool")
        tool = slice_tool_builder(
            kfj.CATEGORIES[category], group, name, indexer, overwrite, arguments, size
        )
        tool.run()
    except AttributeError:
        tool_script = getattr(module, "run")
        tool_script(
            wc.SliceTool(
                kfj.CATEGORIES[category],
                group,
                name,
                indexer,
                overwrite,
                arguments,
                size,
            )
        )
