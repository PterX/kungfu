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

BlackList = ["pnl", "statistics", "group", "name"]


class FeatureStore:
    def __init__(self, stage="prod"):
        self.stage = stage
        self.sso = SSO(stage)
        self.logger = create_logger("featurestore")

        if self.sso.introspect_token() != True:
            self.logger.error("Please Login First, Try kfc login")
            return

        self.sso.get_new_access_token_by_refresh_token()
        phone, username, user_id = self.sso.get_profile()
        self.logger.info(
            f"Feature Store init successfully, phone {phone} username {username}"
        )
        self.user_id = user_id
        self.public_feature_config_map = self.__get_features()
        self.feature_config_map = self.__get_features()

    def login(self, account):
        self.logger.info(f"Welcome, your phone number is: {account}")
        self.sso.send_sms_code(account)
        pass_code = click.prompt("Please enter a valid sms code", type=int)
        self.logger.info(f"Your pass code is: {pass_code}")
        self.sso.sign_in_by_phone_passcode(account, pass_code)
        self.logger.info("Login Success")
        self.public_feature_config_map = self.__get_features()

    def list_public_features(self):
        self.public_feature_config_map = self.__get_features()
        return self.__list_features(self.public_feature_config_map)

    def __list_features(self, feature_map):
        feature_list = []
        for key in feature_map:
            config = feature_map[key]
            name = config.get("name", None)
            feature_list.append({"key": key, "name": name if name else key})
        return feature_list

    def __get_features(self, author="kungfu-trader"):
        access_token, refresh_token, id_token = get_tokens(self.stage)
        headers = {
            "Authorization": id_token,
        }
        payload = {
            "author": author,
        }
        resp = requests.get(
            f"{BASE_URL}/{self.stage}/extensions/", params=payload, headers=headers
        ).text
        resp = json.loads(resp)
        if not isinstance(resp, list):
            raise Exception(f"Get features failed: {resp.get('message', None)}")
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

    def __get_feature_config(self, feature_key, author="kungfu-trader"):
        if author == "kungfu-trader":
            return self.public_feature_config_map.get(feature_key, None)
        else:
            return self.feature_config_map.get(feature_key, None)

    def get_public_feature_date_range(self, feature_key):
        results = self.__get_feature_data_meta(feature_key)

        def get_date(item):
            return datetime.strptime(item["timestamp"], "%Y%m%d").strftime("%Y-%m-%d")

        dates = list(map(get_date, results))
        return dates

    def get_public_feature_data(self, feature_key):
        config = self.__get_feature_config(feature_key)
        if config is None:
            self.logger.error("No configuration for feature {feature_key}")
            return

        results = self.__get_feature_data_meta(feature_key)
        access_key, secret_key, session_token = get_credentials_for_identity(self.stage)
        factors_map = {}

        for item in results:
            self.__deal_result(item, factors_map, access_key, secret_key, session_token)

        return factors_map

    def get_public_feature_data_by_date(self, feature_key, date):
        try:
            date_str = datetime.strptime(date, "%Y-%m-%d").strftime("%Y%m%d")
        except ValueError as e:
            self.logger.error(e)

        config = self.__get_feature_config(feature_key)
        if config is None:
            self.logger.error("No configuration for feature {feature_key}")
            return

        results = self.__get_feature_data_meta(feature_key)

        def filter_date(item):
            return item["timestamp"] == date_str

        after_filter_results = list(filter(filter_date, results))

        if not len(after_filter_results):
            self.logger.error(f"No result of {feature_key} by date {date_str}")
            return

        access_key, secret_key, session_token = get_credentials_for_identity(self.stage)

        factors_map = {}
        for item in after_filter_results:
            self.__deal_result(item, factors_map, access_key, secret_key, session_token)

        return factors_map

    def __deal_result(self, result, factors_map, access_key, secret_key, session_token):
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

            for key in json_content.keys():
                if key in BlackList:
                    continue

                data = json_content[key]
                if isinstance(data, list):
                    df = pd.DataFrame(data)
                elif isinstance(data, dict):
                    df = pd.DataFrame(data, index=[0])
                else:
                    df = pd.DataFrame({key: data}, index=[0])
                df["timestamp"] = timestamp
                if key in factors_map:
                    pre = factors_map[key]
                    factors_map[key] = pd.concat([pre, df]).reset_index(drop=True)
                else:
                    factors_map[key] = df

        except ClientError as err:
            raise err

    def __get_feature_data_meta(self, feature_key, author="kungfu-trader"):
        access_token, refresh_token, id_token = get_tokens(self.stage)
        headers = {
            "Authorization": id_token,
        }
        payload = {
            "author": author,
        }
        resp = requests.get(
            f"{BASE_URL}/{self.stage}/extensions/{feature_key}/results",
            params=payload,
            headers=headers,
        ).text
        resp = json.loads(resp)
        results = resp.get("results", [])

        return sorted(results, key=lambda item: item["timestamp"])

    def list_features(self):
        self.feature_config_map = self.__get_features(self.user_id)
        return self.__list_features(self.feature_config_map)

    def get_feature_date_range(self, feature_key):
        results = self.__get_feature_data_meta(feature_key, self.user_id)

        def get_date(item):
            return datetime.strptime(item["timestamp"], "%Y%m%d").strftime("%Y-%m-%d")

        dates = list(map(get_date, results))
        return dates

    def get_feature_data(self, feature_key):
        config = self.__get_feature_config(feature_key, self.user_id)
        if config is None:
            self.logger.error("No configuration for feature {feature_key}")
            return

        results = self.__get_feature_data_meta(feature_key, self.user_id)
        access_key, secret_key, session_token = get_credentials_for_identity(self.stage)
        factors_map = {}

        for item in results:
            self.__deal_result(item, factors_map, access_key, secret_key, session_token)

        return factors_map

    def get_feature_data_by_date(self, feature_key, date):
        try:
            date_str = datetime.strptime(date, "%Y-%m-%d").strftime("%Y%m%d")
        except ValueError as e:
            self.logger.error(e)

        config = self.__get_feature_config(feature_key, self.user_id)
        if config is None:
            self.logger.error("No configuration for feature {feature_key}")
            return

        results = self.__get_feature_data_meta(feature_key, self.user_id)

        def filter_date(item):
            return item["timestamp"] == date_str

        after_filter_results = list(filter(filter_date, results))

        if not len(after_filter_results):
            self.logger.error(f"No result of {feature_key} by date {date_str}")
            return

        access_key, secret_key, session_token = get_credentials_for_identity(self.stage)

        factors_map = {}
        for item in after_filter_results:
            self.__deal_result(item, factors_map, access_key, secret_key, session_token)

        return factors_map
