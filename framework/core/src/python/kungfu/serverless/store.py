import boto3
import json
from kungfu.serverless.sso import SSO
from kungfu.serverless.utils import get_tokens
from kungfu.serverless.config import BASE_URL
import click
from datetime import datetime
import requests
import sys
import warnings

class FeatureStore:
    def __init__(self, stage="prod"):
        self.stage = stage
        self.sso = SSO(stage)

        if self.sso.introspect_token() != True:
            print("Please Login First, Try kfc login")
            return

        self.sso.get_new_access_token_by_refresh_token()
        phone, username, user_id = self.sso.get_profile()
        print(f"Feature Store init successfully, phone {phone} username {username}")
        self.user_id = user_id
        
    def login(self, account):
        print("Welcome, your phone number is: ", account)
        self.sso.send_sms_code(account)
        pass_code = click.prompt("Please enter a valid sms code", type=int)
        print("Your pass code is: ", pass_code)
        self.sso.sign_in_by_phone_passcode(account, pass_code)
        print("Login Success")
        
    def list_public_features(self):
        feature_map = self.__get_public_features()
        feature_list = []
        for key in feature_map:
            config = feature_map[key]
            name = config.get("name", None)
            feature_list.append({
                "key": key,
                "name": name if name else key
            })
            
        return feature_list
    
    def __get_public_features(self):
        access_token, refresh_token, id_token = get_tokens(self.stage)
        headers = {
            "Authorization": id_token,
        }
        payload = {
            "author": 'kungfu-trader',
        }
        resp = requests.get(f"{BASE_URL}/{self.stage}/extensions/", params=payload, headers=headers).text
        resp = json.loads(resp)
        if not isinstance(resp, list):
            raise Exception(f"Get public features failed: {resp.get('message', None)}")
        features_map = self.__build_feature_map(resp)
        return features_map
        
    
    def __build_feature_map(self, origin_data):
        feature_map = {}
        platform = sys.platform
        for item in origin_data:
            packages = item.get('packages', None)
            if packages is None:
                warnings.warn(f"{item.module_name} does not have packages")
                continue
            
            for p in packages:
                # if p["platform"] == platform:
                config = p["kungfuConfig"]
                feature_map[item["module_name"]] = config
        return feature_map
    
    def get_public_feature_date_range(self, feature_key):
        results = self.get_public_feature_data(feature_key)
        
        def get_date(item):
            return datetime.strptime(item["timestamp"], "%Y%m%d")
        
        dates = list(map(get_date, results))        
        return dates
    
    def get_public_feature_data(self, feature_key):
        results = self.get_public_feature_data(feature_key)
        for item in results:
            result = item["result"]
            
            
        return results
    
    def __get_public_feature_date_meta(self, feature_key):
    
    
    def list_features(self):
        print("We are working on the following features")
        pass
    
    def get_feature_date_range(self, feature_key):
        print("We are working on the following features")
        pass
    
    def get_feature_data(self, feature_key, begin_time, end_time):
        print("We are working on the following features")
        pass