import click
import json

from kungfu.console.commands import kfc, PrioritizedCommandGroup
from kungfu.serverless.backtest import Backtest
from kungfu.serverless.utils import create_logger

backtest_command_context = kfc.pass_context("backtest_client", "logger")


@kfc.group(cls=PrioritizedCommandGroup, help_priority=2)
@kfc.pass_context()
def backtest(ctx):
    ctx.logger = create_logger("backtest_command")
    ctx.backtest_client = Backtest(ctx.stage)
    pass


@backtest.command()
@click.option(
    "-f", "--file_path", type=str, required=True, help="full path of backtest file"
)
@click.option(
    "-b",
    "--begin_time",
    type=click.DateTime(["%Y-%m-%d", "%Y%m%d"]),
    required=True,
    help="backest begin time",
)
@click.option(
    "-e",
    "--end_time",
    type=click.DateTime(["%Y-%m-%d", "%Y%m%d"]),
    required=True,
    help="backest end time",
)
@click.option(
    "-l",
    "--data_level",
    type=click.Choice(["level1", "level2"]),
    default="level1",
    required=True,
    help="level1 or level2 data source for backtest",
)
@backtest_command_context
def submit(ctx, file_path, begin_time, end_time, data_level):
    result = ctx.backtest_client.submit(file_path, begin_time, end_time, data_level)
    ctx.logger.info(
        " >>>>>>>>>>>>>>>>>>>>>>>>>>>>> backtest result <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
    )
    ctx.logger.info(f"{json.dumps(result)}")
    ctx.logger.info(
        " >>>>>>>>>>>>>>>>>>>>>>>>>>>>> backtest result <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"
    )
    pass


@backtest.command()
@backtest_command_context
def datarange(ctx):
    categories = ctx.backtest_client.check_data_range()
    ctx.logger.info("Support Daterange for L2_Quote, L2_Order, L2_Tick: ")
    for key in categories.keys():
        ctx.logger.info(f"{key}")
        for item in categories[key]:
            ctx.logger.info(
                f"  {item['security_tyep']} {item['exchange']} start {item['start_time']} end {item['end_time']}"
            )
        ctx.logger.info("\n")
