import boto3
import json
from kungfu.serverless.sso import SSO
from kungfu.serverless.utils import get_credentials_for_identity


class FeatureStore:
    def __init__(self, stage="prod"):
        self.stage = stage
        self.sso = SSO(stage)

        if self.sso.introspect_token() != True:
            print("Please Login First, Try kfc login")
            raise Exception("Login Required, Try kfc login")

        self.sso.get_new_access_token_by_refresh_token()
        phone, username = self.sso.get_profile()
        print(f"Feature Store init successfully, phone {phone} username {username}")

    def list_buckets(self):
        access_key, secret_key, session_token = get_credentials_for_identity(self.stage)
        s3 = boto3.client(
            "s3",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )
        resp = s3.list_buckets()
        return resp["Buckets"]

    def list_objects(self, bucket):
        access_key, secret_key, session_token = get_credentials_for_identity(self.stage)
        s3 = boto3.client(
            "s3",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )
        resp = s3.list_objects(Bucket=bucket)
        return resp["Contents"]

    def get_object(self, bucket, key):
        access_key, secret_key, session_token = get_credentials_for_identity(self.stage)
        s3 = boto3.client(
            "s3",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )
        resp = s3.get_object(Bucket=bucket, Key=key)
        my_json = json.loads(resp["Body"].read())
        return my_json
