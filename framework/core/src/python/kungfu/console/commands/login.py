from kungfu.console.commands import kfc
from kungfu.serverless.sso import SSO
from kungfu.serverless.utils import create_logger
import click


@kfc.command(help_priority=1)
@click.option(
    "-A", "--account", type=str, help="account phone number of kungfu account"
)
@kfc.pass_context()
def login(ctx, account):
    logger = create_logger("login")
    logger.info(f"Welcome, your phone number is: {account}")
    sso = SSO(ctx.stage)
    sso.send_sms_code(account)
    pass_code = click.prompt("Please enter a valid sms code", type=int)
    logger.info(f"Your pass code is: {pass_code}")
    sso.sign_in_by_phone_passcode(account, pass_code)
    logger.info("Login Success")
