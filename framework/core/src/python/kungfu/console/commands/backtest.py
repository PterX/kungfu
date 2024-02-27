import click
from kungfu.console.commands import kfc, PrioritizedCommandGroup
from kungfu.serverless.backtest import Backtest


@kfc.group(cls=PrioritizedCommandGroup, help_priority=2)
@kfc.pass_context()
def backtest(ctx):
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
@kfc.pass_context()
def submit(ctx, file_path, begin_time, end_time, data_level):
    ctx.backtest_client = Backtest(ctx.stage)
    ctx.backtest_client.submit(file_path, begin_time, end_time, data_level)
    pass
