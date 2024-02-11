from authing import AuthenticationClient
from kungfu.store.app_config import authing_app_config
from kungfu.yijinjing.utils import get_default_home_dir
import os
import json
import boto3
from urllib.parse import urlparse

client = boto3.client("cognito-identity", region_name="cn-north-1")


def ensure_json(file_path):
    if not os.path.exists(file_path):
        with open(file_path, "w") as file:
            json.dump({}, file)


def write_json(file_path, data={}):
    with open(file_path, "w") as file:
        json.dump(data, file)


def get_sls_kungfu_params(stage):
    json_path = os.path.join(os.path.dirname(__file__), "params", f"{stage}.json")
    with open(json_path, "r") as params_file:
        return json.loads(params_file.read())


class SSO:
    def __init__(self, stage="prod", kf_home=get_default_home_dir()):
        self.stage = stage
        self.sls_params = get_sls_kungfu_params(stage)
        self.ac = AuthenticationClient(
            app_id=authing_app_config[self.stage]["appId"],
            app_host=authing_app_config[self.stage]["appHost"],
            app_secret=authing_app_config[self.stage]["appSecret"],
        )

        self.token_file = os.path.join(kf_home, "auth_token.json")
        access_token, refresh_token, id_token = self.__get_tokens()
        self.access_token = access_token
        self.refresh_token = refresh_token
        self.id_token = id_token

    def sign_in_by_account_password(self, account, password):
        sign_in_resp = self.ac.sign_in_by_account_password(
            account,
            password,
            options={"scope": "phone profile email openid offline_access backtest"},
        )

        if sign_in_resp["statusCode"] != 200:
            print("Login Error", sign_in_resp["statusCode"], sign_in_resp["message"])
            return

        print("Login Success")
        access_token = sign_in_resp["data"]["access_token"]
        refresh_token = sign_in_resp["data"]["refresh_token"]
        id_token = sign_in_resp["data"]["id_token"]
        self.__record_tokens(access_token, refresh_token, id_token)

    def send_sms_code(self, phone_number):
        self.ac.send_sms(channel="CHANNEL_LOGIN", phone_number=phone_number)

    def sign_in_by_phone_passcode(self, phone, pass_code):
        sign_in_resp = self.ac.sign_in_by_phone_passcode(
            phone=phone,
            pass_code=str(pass_code),
            options={"scope": "phone profile email openid offline_access backtest"},
        )

        if sign_in_resp["statusCode"] != 200:
            print("Login Error", sign_in_resp["statusCode"], sign_in_resp["message"])
            return

        access_token = sign_in_resp["data"]["access_token"]
        refresh_token = sign_in_resp["data"]["refresh_token"]
        id_token = sign_in_resp["data"]["id_token"]
        self.__record_tokens(access_token, refresh_token, id_token)

    def get_new_access_token_by_refresh_token(self):
        get_access_token_resp = self.ac.get_new_access_token_by_refresh_token(
            self.refresh_token
        )

        if get_access_token_resp.get("error", None) is not None:
            print(
                "Get New Access Token Error:",
                get_access_token_resp["error"],
                get_access_token_resp["error_description"],
            )
            return

        print("Get New Access Token Success")
        access_token = get_access_token_resp["access_token"]
        refresh_token = get_access_token_resp["refresh_token"]
        id_token = get_access_token_resp["id_token"]
        self.__record_tokens(access_token, refresh_token, id_token)

    def __record_tokens(self, access_token, refresh_token, id_token):
        self.access_token = access_token
        self.refresh_token = refresh_token
        self.id_token = id_token
        write_json(
            self.token_file,
            {
                "access_token": self.access_token,
                "refresh_token": self.refresh_token,
                "id_token": self.id_token,
            },
        )

    def __get_tokens(self):
        ensure_json(self.token_file)
        with open(self.token_file, "r") as file:
            loaded_data = json.load(file)
            return (
                loaded_data.get("access_token", ""),
                loaded_data.get("refresh_token", ""),
                loaded_data.get("id_token", ""),
            )

    def get_credentials_for_identity(self):
        host_name = urlparse(authing_app_config[self.stage]["appHost"]).netloc
        login_info = {[f"{host_name}\oidc"]: self.id_token}
        identity_id_resp = client.get_id(
            IdentityPoolId=self.sls_params["identity_pool_id"]["value"],
            Logins=login_info,
        )
        identity_id = identity_id_resp["IdentityId"]
        resp = client.get_credentials_for_identity(
            IdentityId=identity_id, Logins=login_info
        )

        credentials = resp["Credentials"]
        return (
            credentials["AccessKeyId"],
            credentials["SecretKey"],
            credentials["SessionToken"],
        )

    def list_buckets(self, access_key, secret_key, session_token):
        s3 = boto3.client(
            "s3",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )
        resp = s3.list_buckets()
        return resp["Buckets"]

    def list_objects(self, bucket, access_key, secret_key, session_token):
        s3 = boto3.client(
            "s3",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )
        resp = s3.list_objects(Bucket=bucket)
        return resp["Contents"]

    def get_object(self, bucket, key, access_key, secret_key, session_token):
        s3 = boto3.client(
            "s3",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )
        resp = s3.get_object(Bucket=bucket, Key=key)
        my_json = json.loads(resp["Body"].read())
        print(my_json, "====")


# sso = SSO("alpha")
# sso.sign_in_by_account_password(account="13151998870", password="")
# AccessKeyId, SecretKey, SessionToken = sso.get_credentials_for_identity()
# buckets = sso.list_buckets(AccessKeyId, SecretKey, SessionToken)
# objects = sso.list_objects("kungfu", AccessKeyId, SecretKey, SessionToken)
# object = sso.get_object("kungfu", objects[0]["Key"], AccessKeyId, SecretKey, SessionToken)
