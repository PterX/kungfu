from authing import AuthenticationClient
from kungfu.serverless.config import AUTHING_APP_CONFIG
from kungfu.serverless.utils import record_tokens, get_tokens
from kungfu.serverless.utils import create_logger


class SSO:
    def __init__(self, stage="prod"):
        self.logger = create_logger("sso")
        self.stage = stage
        self.ac = AuthenticationClient(
            app_id=AUTHING_APP_CONFIG[self.stage]["appId"],
            app_host=AUTHING_APP_CONFIG[self.stage]["appHost"],
            app_secret=AUTHING_APP_CONFIG[self.stage]["appSecret"],
        )

    def get_profile(self):
        access_token, refresh_token, id_token = get_tokens(self.stage)
        self.ac.set_access_token(access_token)
        resp = self.ac.get_profile()
        if resp["statusCode"] != 200:
            self.logger.exception(
                "Get Profile Failed", resp["statusCode"], resp["message"]
            )
            raise Exception("Get Profile Failed")

        data = resp["data"]
        phone = data["phone"]
        username = data["username"]
        user_id = data["userId"]
        return phone, username, user_id

    def sign_in_by_account_password(self, account, password):
        sign_in_resp = self.ac.sign_in_by_account_password(
            str(account),
            str(password),
            options={"scope": "phone profile email openid offline_access backtest"},
        )

        if sign_in_resp["statusCode"] != 200:
            self.logger.error(
                f"Login Error f{sign_in_resp['statusCode']}, f{sign_in_resp['message']}"
            )
            return

        self.logger.info("Login Success")
        access_token = sign_in_resp["data"]["access_token"]
        refresh_token = sign_in_resp["data"]["refresh_token"]
        id_token = sign_in_resp["data"]["id_token"]
        expires_in = sign_in_resp["data"]["expires_in"]
        record_tokens(self.stage, access_token, refresh_token, id_token, expires_in)

    def send_sms_code(self, phone_number):
        self.ac.send_sms(channel="CHANNEL_LOGIN", phone_number=str(phone_number))

    def sign_in_by_phone_passcode(self, phone, pass_code):
        sign_in_resp = self.ac.sign_in_by_phone_passcode(
            phone=str(phone),
            pass_code=str(pass_code),
            options={"scope": "phone profile email openid offline_access backtest"},
        )

        if sign_in_resp["statusCode"] != 200:
            self.logger.error(
                f"Login Error, {sign_in_resp['statusCode']}, {sign_in_resp['message']}"
            )
            return

        access_token = sign_in_resp["data"]["access_token"]
        refresh_token = sign_in_resp["data"]["refresh_token"]
        id_token = sign_in_resp["data"]["id_token"]
        expires_in = sign_in_resp["data"]["expires_in"]
        record_tokens(self.stage, access_token, refresh_token, id_token, expires_in)

    def get_new_access_token_by_refresh_token(self):
        access_token, refresh_token, id_token = get_tokens(self.stage)
        get_access_token_resp = self.ac.get_new_access_token_by_refresh_token(
            refresh_token
        )

        if get_access_token_resp.get("error", None) is not None:
            self.logger.error(
                f"Get New Access Token Error: {get_access_token_resp['error']}, {get_access_token_resp['error_description']}"
            )
            return

        access_token = get_access_token_resp["access_token"]
        refresh_token = get_access_token_resp["refresh_token"]
        id_token = get_access_token_resp["id_token"]
        expires_in = get_access_token_resp["expires_in"]
        record_tokens(self.stage, access_token, refresh_token, id_token, expires_in)

    def introspect_token(self):
        access_token, refresh_token, id_token = get_tokens(self.stage)
        resp = self.ac.introspect_token(access_token)
        if resp.get("active", False) == True:
            return True
        else:
            return False
