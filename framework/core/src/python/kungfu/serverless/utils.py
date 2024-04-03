from kungfu.serverless.config import TOKEN_FILE, APP_PARAMS, AUTHING_APP_CONFIG
import boto3
from botocore.exceptions import ClientError
import json
import os
import time
from urllib.parse import urlparse
import logging
from kungfu.yijinjing.log import LOG_LEVELS


def record_tokens(stage, access_token, refresh_token, id_token, expires_in):
    write_token_json(
        TOKEN_FILE,
        stage,
        {
            "access_token": access_token,
            "refresh_token": refresh_token,
            "id_token": id_token,
            "expires_in": expires_in,
            "gen_time": int(time.time()),
        },
    )


def get_tokens(stage):
    ensure_token_json(TOKEN_FILE)
    with open(TOKEN_FILE, "r") as file:
        total_data = json.load(file)
        loaded_data = total_data.get(stage, {})
        return (
            loaded_data.get("access_token", ""),
            loaded_data.get("refresh_token", ""),
            loaded_data.get("id_token", ""),
        )


def ensure_token_json(file_path):
    if not os.path.exists(file_path):
        with open(file_path, "w") as file:
            json.dump({}, file)


def write_token_json(file_path, stage, data={}):
    with open(TOKEN_FILE, "r") as file:
        total_data = json.load(file)

    with open(file_path, "w") as file:
        total_data[stage] = data
        json.dump(total_data, file)


def get_sls_kungfu_params(stage):
    return APP_PARAMS[stage]


def get_credentials_for_identity(stage):
    client = boto3.client("cognito-identity", region_name="cn-north-1")
    host_name = urlparse(AUTHING_APP_CONFIG[stage]["appHost"]).netloc
    access_token, refresh_token, id_token = get_tokens(stage)
    login_info = {f"{host_name}/oidc": id_token}
    identity_pool_id = get_sls_kungfu_params(stage)["identity_pool_id"]["value"]

    try:
        identity_id_resp = client.get_id(
            IdentityPoolId=identity_pool_id,
            Logins=login_info,
        )
    except ClientError as err:
        raise err

    identity_id = identity_id_resp["IdentityId"]

    try:
        resp = client.get_credentials_for_identity(
            IdentityId=identity_id, Logins=login_info
        )
    except ClientError as err:
        raise err

    credentials = resp["Credentials"]
    return (
        credentials["AccessKeyId"],
        credentials["SecretKey"],
        credentials["SessionToken"],
    )


def read_file_content(filename):
    with open(filename, "r") as file:
        content = file.read()
    return content


def create_logger(name, level="debug"):
    logger = logging.getLogger(name)
    handler = logging.StreamHandler
    logger.addHandler(handler())
    logger.setLevel(LOG_LEVELS[level])
    return logger
