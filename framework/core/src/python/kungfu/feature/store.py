import boto3
import json
from urllib.parse import urlparse
from kungfu.feature.config import AUTHING_APP_CONFIG
from kungfu.feature.utils import get_sls_kungfu_params, get_tokens

client = boto3.client("cognito-identity", region_name="cn-north-1")


class FeatureStore:
    def __init__(self, stage="prod"):
        self.stage = stage
        self.sls_params = get_sls_kungfu_params(stage)

    def get_credentials_for_identity(self):
        host_name = urlparse(AUTHING_APP_CONFIG[self.stage]["appHost"]).netloc
        access_token, refresh_token, id_token = get_tokens()
        login_info = {f"{host_name}/oidc": id_token}
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
        return my_json