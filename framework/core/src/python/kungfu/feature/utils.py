from kungfu.feature.config import TOKEN_FILE, APP_PARAMS

import json
import os


def record_tokens(access_token, refresh_token, id_token):
    write_token_json(
        TOKEN_FILE,
        {
            "access_token": access_token,
            "refresh_token": refresh_token,
            "id_token": id_token,
        },
    )


def get_tokens():
    ensure_token_json(TOKEN_FILE)
    with open(TOKEN_FILE, "r") as file:
        loaded_data = json.load(file)
        return (
            loaded_data.get("access_token", ""),
            loaded_data.get("refresh_token", ""),
            loaded_data.get("id_token", ""),
        )


def ensure_token_json(file_path):
    if not os.path.exists(file_path):
        with open(file_path, "w") as file:
            json.dump({}, file)


def write_token_json(file_path, data={}):
    with open(file_path, "w") as file:
        json.dump(data, file)


def get_sls_kungfu_params(stage):
    return APP_PARAMS[stage]
