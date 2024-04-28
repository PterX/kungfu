from kungfu.serverless.config import TOKEN_FILE, APP_PARAMS, AUTHING_APP_CONFIG
import boto3
from botocore.exceptions import ClientError
import json
import os
import time
from urllib.parse import urlparse
import logging
import tarfile
from kungfu.yijinjing.log import LOG_LEVELS

UPLOAD_EXT_WHITELIST = [".csv", ".xlsx", ".json", ".py", ".so"]

MB = 1024 * 1024

UPLOAD_DIR_SIZE_LIMIT_MB = 100


def make_tarfile(output_filename, source_dir):
    output_filename = os.path.join(source_dir, output_filename + ".tar.gz")

    def filter_whitelist(tarinfo):
        filename = tarinfo.name
        print(filename)
        print(is_end_with(filename, UPLOAD_EXT_WHITELIST))
        if tarinfo.isfile() and not is_end_with(filename, UPLOAD_EXT_WHITELIST):
            return None
        return tarinfo

    with tarfile.open(output_filename, "w:gz") as tar:
        tar.add(
            source_dir, arcname=os.path.basename(source_dir), filter=filter_whitelist
        )
    return output_filename


def get_dir_size(dir_path):
    size = 0
    for root, dirnames, filenames in os.walk(dir_path):
        for f in filenames:
            if not is_end_with(f, UPLOAD_EXT_WHITELIST):
                continue
            size += os.path.getsize(os.path.join(root, f))
    return size / MB


def is_end_with(filename, supported_ext=[]):
    result = False
    for ext in supported_ext:
        if filename.endswith(ext):
            result = True
            break
    return result


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


def create_logger(name, level="debug"):
    logger = logging.getLogger(name)
    handler = logging.StreamHandler
    logger.addHandler(handler())
    logger.setLevel(LOG_LEVELS[level])
    return logger


def read_file_content(filename):
    with open(filename, "rb") as file:
        content = file.read()
    return content


def read_zip(filename):
    with open(filename, "rb") as file:
        return file


def build_backtest_json(file_path, module_name):
    folder = os.path.dirname(file_path)
    target_json = os.path.join(folder, "package.json")
    j = {}
    if os.path.exists(target_json):
        with open(target_json, "r") as target_json_file:
            j = json.load(target_json_file)

    bn = os.path.basename(file_path)
    j["name"] = module_name
    j["description"] = j.get("description", "")
    j["version"] = j.get("version", "0.0.0")
    j["main"] = os.path.basename(bn)
    j["binary"] = j.get("binary", {"module_name": module_name})
    return j
