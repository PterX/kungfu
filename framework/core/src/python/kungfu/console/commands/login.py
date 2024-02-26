from kungfu.console.commands import kfc
from kungfu.serverless.sso import SSO
import click


@kfc.command(help_priority=1)
@click.option(
    "-A", "--account", type=str, help="account phone number of kungfu account"
)
@kfc.pass_context()
def login(ctx, account):
    print("Welcome, your phone number is: ", account)
    sso = SSO(ctx.stage)
    sso.send_sms_code(account)
    pass_code = click.prompt("Please enter a valid sms code", type=int)
    print("Your pass code is: ", pass_code)
    sso.sign_in_by_phone_passcode(account, pass_code)
    print("Login Success")
