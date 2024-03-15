import boto3
from botocore.exceptions import ClientError
import json
from kungfu.serverless.sso import SSO
from kungfu.serverless.utils import get_tokens, get_credentials_for_identity
from kungfu.serverless.config import BASE_URL
import click
from datetime import datetime
import requests
import sys
import logging
import pandas as pd
from kungfu.serverless.utils import create_logger


class FeatureStore:
    def __init__(self, stage="prod"):
        self.stage = stage
        self.sso = SSO(stage)
        self.logger = create_logger("featureStore")
    

        if self.sso.introspect_token() != True:
            self.logger.error("Please Login First, Try kfc login")
            return

        self.sso.get_new_access_token_by_refresh_token()
        phone, username, user_id = self.sso.get_profile()
        self.logger.info(
            f"Feature Store init successfully, phone {phone} username {username}"
        )
        self.user_id = user_id
        self.feature_config_map = self.__get_public_features()

    def login(self, account):
        self.logger.info(f"Welcome, your phone number is: {account}")
        self.sso.send_sms_code(account)
        pass_code = click.prompt("Please enter a valid sms code", type=int)
        self.logger.info(f"Your pass code is: {pass_code}")
        self.sso.sign_in_by_phone_passcode(account, pass_code)
        self.logger.info("Login Success")
        self.feature_config_map = self.__get_public_features()

    def list_public_features(self):
        feature_list = []
        for key in self.feature_config_map:
            config = self.feature_config_map[key]
            name = config.get("name", None)
            feature_list.append({"key": key, "name": name if name else key})

        return feature_list

    def __get_public_features(self):
        access_token, refresh_token, id_token = get_tokens(self.stage)
        headers = {
            "Authorization": id_token,
        }
        payload = {
            "author": "kungfu-trader",
        }
        resp = requests.get(
            f"{BASE_URL}/{self.stage}/extensions/", params=payload, headers=headers
        ).text
        resp = json.loads(resp)
        if not isinstance(resp, list):
            raise Exception(f"Get public features failed: {resp.get('message', None)}")
        features_map = self.__build_feature_map(resp)
        return features_map

    def __build_feature_map(self, origin_data):
        feature_config_map = {}
        platform = sys.platform
        for item in origin_data:
            packages = item.get("packages", None)
            if packages is None:
                self.logger.warn(f"{item.module_name} does not have packages")
                continue

            for p in packages:
                # if p["platform"] == platform:
                config = p["kungfuConfig"]
                feature_config_map[item["module_name"]] = config
        return feature_config_map

    def __get_feature_config(self, feature_key):
        return self.feature_config_map.get(feature_key, None)

    def get_public_feature_date_range(self, feature_key):
        results = self.__get_public_feature_date_meta(feature_key)

        def get_date(item):
            return datetime.strptime(item["timestamp"], "%Y%m%d")

        dates = list(map(get_date, results))
        return dates

    def get_public_feature_data(self, feature_key):
        config = self.__get_feature_config(feature_key)
        if config is None:
            self.logger.error("No configuration for feature {feature_key}")
            return

        results = self.__get_public_feature_date_meta(feature_key)
        access_key, secret_key, session_token = get_credentials_for_identity(self.stage)
        factors_map = {}

        for item in results:
            self.__deal_result(
                item, feature_key, factors_map, access_key, secret_key, session_token
            )

        return factors_map

    def __deal_result(
        self, result, feature_key, factors_map, access_key, secret_key, session_token
    ):
        result_url = result["result"]
        bucket_name, object_key = result_url.replace("s3://", "").split("/", 1)
        s3_client = boto3.client(
            "s3",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )
        timestamp = result["timestamp"]
        try:
            response = s3_client.get_object(Bucket=bucket_name, Key=object_key)
            json_content = json.loads(response["Body"].read().decode("utf-8"))

            config = self.feature_config_map[feature_key]
            factors = config["factors"]
            for f in factors:
                context_key = f["contextKey"]
                data = json_content[context_key]
                df = pd.DataFrame(data)
                df["timestamp"] = timestamp
                if context_key in factors_map:
                    pre = factors_map[context_key]
                    factors_map[context_key] = pd.concat([pre, df]).reset_index(
                        drop=True
                    )
                else:
                    factors_map[context_key] = df

        except ClientError as err:
            raise err

    def __get_public_feature_date_meta(self, feature_key):
        access_token, refresh_token, id_token = get_tokens(self.stage)
        headers = {
            "Authorization": id_token,
        }
        payload = {
            "author": "kungfu-trader",
        }
        resp = requests.get(
            f"{BASE_URL}/{self.stage}/extensions/{feature_key}/results",
            params=payload,
            headers=headers,
        ).text
        resp = json.loads(resp)
        results = resp.get("results", [])
        return results

    def list_features(self):
        print("We are working on the following features")
        pass

    def get_feature_date_range(self, feature_key):
        print("We are working on the following features")
        pass

    def get_feature_data(self, feature_key, begin_time, end_time):
        print("We are working on the following features")
        pass
