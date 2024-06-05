import boto3
from botocore.exceptions import ClientError
import os
import json
import kungfu
import requests
import time
import signal
import sys
import io
import tarfile

from kungfu.serverless.sso import SSO
from kungfu.serverless.utils import (
    get_credentials_for_identity,
    read_zip,
    get_tokens,
    UPLOAD_EXT_WHITELIST,
    UPLOAD_DIR_SIZE_LIMIT_MB,
    get_dir_size,
    make_tarfile,
    build_backtest_json,
)
from kungfu.serverless.config import BASE_URL
from kungfu.serverless.utils import create_logger

yjj = kungfu.__binding__.yijinjing


class Backtest:
    def __init__(self, stage="prod"):
        self.stage = stage
        self.sso = SSO(stage)
        self.logger = create_logger("backtest")

        if self.sso.introspect_token() != True:
            self.logger.error("Please Login First, Try kfc login")
            return

        self.sso.get_new_access_token_by_refresh_token()
        phone, username, user_id = self.sso.get_profile()
        self.logger.info(
            f"Backtest init successfully, phone {phone} username {username} userid {user_id}"
        )
        self.user_id = user_id

        (
            JOB_DEFINITION_ARN_PARAM_NAME,
            JOB_QUEUE_ARN_PARAM_NAME,
            S3_BUCKET_PARAM_NAME,
            LOG_GROUP_PARAM_NAME,
        ) = self.__get_params_name()
        self.JOB_DEFINITION_ARN_PARAM_NAME = JOB_DEFINITION_ARN_PARAM_NAME
        self.JOB_QUEUE_ARN_PARAM_NAME = JOB_QUEUE_ARN_PARAM_NAME
        self.S3_BUCKET_PARAM_NAME = S3_BUCKET_PARAM_NAME
        self.LOG_GROUP_PARAM_NAME = LOG_GROUP_PARAM_NAME

    def submit(self, file_path, begin_time, end_time, level="level1"):
        access_key, secret_key, session_token = get_credentials_for_identity(self.stage)
        parameter_map = self.__get_params(access_key, secret_key, session_token)
        file_basename = os.path.basename(file_path)
        file_name, suffix = os.path.splitext(file_basename)
        module_name = f"strategy{file_name}{int(time.time()* 10000)}"
        dirname = os.path.dirname(file_path)

        self.logger.warning(
            f"Only submit file endswith {', '.join(UPLOAD_EXT_WHITELIST)}"
        )
        dir_size = get_dir_size(dirname)

        if dir_size > UPLOAD_DIR_SIZE_LIMIT_MB:
            raise Exception(
                f"Folder {os.path.dirname(file_path) }of {file_path} exceeds {UPLOAD_DIR_SIZE_LIMIT_MB}MB"
            )
        packagejson = build_backtest_json(file_path, module_name)
        packagejson_path = os.path.join(dirname, "package.json")
        with open(packagejson_path, "w") as pj:
            json.dump(packagejson, pj)

        zip_file = make_tarfile(module_name, os.path.dirname(file_path))
        self.logger.info(f"tmp zip file: {zip_file}")
        self.__put_resource(
            zip_file, parameter_map, access_key, secret_key, session_token
        )
        self.logger.info(f"remove tmp zip file: {zip_file}")
        os.remove(zip_file)
        job_id = self.__run_job(zip_file, begin_time, end_time, level, parameter_map)
        log_group_name = parameter_map[self.LOG_GROUP_PARAM_NAME]
        self.__monit_log(log_group_name, job_id, access_key, secret_key, session_token)
        backtest_result_bucket = parameter_map[self.S3_BUCKET_PARAM_NAME]
        result = self.__get_backtest_result(
            job_id, backtest_result_bucket, access_key, secret_key, session_token
        )

        return result

    def __get_backtest_result(
        self, job_id, bucket_name, access_key, secret_key, session_token
    ):
        client = boto3.client(
            "s3",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )
        key = f"{self.user_id}/download/{job_id}.tar.gz"
        result_file = io.BytesIO()
        client.download_fileobj(Bucket=bucket_name, Key=key, Fileobj=result_file)
        result_file.seek(0)

        with tarfile.open(fileobj=result_file, mode="r:gz") as tar:
            for name in tar.getnames():
                if "context_dump.json" in name:
                    file_content_byte = tar.extractfile(name).read().decode("utf-8")
                    result = json.loads(file_content_byte)
                    return result
        raise Exception(f"No context_dump.json found, bucket {bucket_name}, key {key}")

    def check_data_range(self):
        access_token, refresh_token, id_token = get_tokens(self.stage)
        resp = requests.get(
            f"{BASE_URL}/{self.stage}/dataset/meta",
            headers={
                "Authorization": id_token,
            },
        ).text

        categories = {}
        datasets = json.loads(resp)[0]["subsets"]
        for item in datasets:
            security_tyep = item["securitytype"]
            exchange = item["exchange"]
            category = item["category"]
            start_time = item["startdate"]
            end_time = item["enddate"]

            categories[category] = (
                [] if not categories.get(category, None) else categories[category]
            )
            categories[category].append(
                {
                    "security_tyep": security_tyep,
                    "exchange": exchange,
                    "category": category,
                    "start_time": start_time,
                    "end_time": end_time,
                }
            )
        return categories

    def __get_params_name(self):
        JOB_DEFINITION_ARN_PARAM_NAME = (
            f"/serverless-kungfu/backtest/{self.stage}/job_definition_arn"
        )
        JOB_QUEUE_ARN_PARAM_NAME = (
            f"/serverless-kungfu/backtest/{self.stage}/job_queue_arn"
        )
        S3_BUCKET_PARAM_NAME = f"/serverless-kungfu/backtest/{self.stage}/s3_bucket"
        LOG_GROUP_PARAM_NAME = (
            f"/serverless-kungfu/backtest/{self.stage}/log_group_name"
        )
        return (
            JOB_DEFINITION_ARN_PARAM_NAME,
            JOB_QUEUE_ARN_PARAM_NAME,
            S3_BUCKET_PARAM_NAME,
            LOG_GROUP_PARAM_NAME,
        )

    def __get_params(self, access_key, secret_key, session_token):
        client = boto3.client(
            "ssm",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )
        try:
            resp = client.get_parameters(
                Names=[
                    self.JOB_DEFINITION_ARN_PARAM_NAME,
                    self.JOB_QUEUE_ARN_PARAM_NAME,
                    self.S3_BUCKET_PARAM_NAME,
                    self.LOG_GROUP_PARAM_NAME,
                ]
            )
        except ClientError as err:
            raise err

        parameters = resp["Parameters"]
        parameter_map = {}
        for item in parameters:
            parameter_map[item["Name"]] = item["Value"]

        return parameter_map

    def __put_resource(
        self, file_path, parameter_map, access_key, secret_key, session_token
    ):
        file_basename = os.path.basename(file_path)

        objectKey = f"{self.user_id}/upload/{file_basename}"
        s3 = boto3.client(
            "s3",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )

        bucket_name = parameter_map[self.S3_BUCKET_PARAM_NAME]

        try:
            s3.upload_file(file_path, bucket_name, objectKey)
        except ClientError as err:
            raise err

    def __run_job(self, file_path, begin_time, end_time, level, parameter_map):
        file_basename = os.path.basename(file_path)
        data = {
            "jobName": "default",
            "parameters": {
                "filename": file_basename,
                "begin_time": begin_time.strftime("%Y-%m-%d"),
                "end_time": end_time.strftime("%Y-%m-%d"),
                "data_categories": "L2" if level == "level1" else "L2,order,tick",
            },
        }
        access_token, refresh_token, id_token = get_tokens(self.stage)
        headers = {
            "Content-Type": "application/json",
            "Authorization": id_token,
        }
        resp = requests.post(
            f"{BASE_URL}/{self.stage}/backtest/submitjob",
            data=json.dumps(data),
            headers=headers,
        ).text
        resp = json.loads(resp)
        jobId = resp.get("jobId", None)
        if not jobId:
            self.logger.error(f"Job sumbitted failed: {resp.text['message']}")
            raise Exception("Job sumbitted failed")

        self.logger.info(f"Job sumbitted, id: {jobId}")
        return jobId

    def __monit_log(
        self, log_group_name, job_id, access_key, secret_key, session_token
    ):
        batch_client = boto3.client(
            "batch",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )
        logs_client = boto3.client(
            "logs",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )

        def try_exit(signum, frame):
            try:
                self.logger.warning(f"job {job_id} exiting...")
                batch_client.terminate_job(jobId=job_id, reason="user triggered")
                self.logger.warning(f"job {job_id} exited successfully")
            except ClientError as err:
                self.logger.exception(err)

            sys.exit()

        signal.signal(signal.SIGINT, try_exit)
        signal.signal(signal.SIGTERM, try_exit)

        next_token = ""
        start_time = time.time()

        while True:
            try:
                resp = batch_client.describe_jobs(jobs=[job_id])
            except ClientError as err:
                self.logger.exception(f"describe job failed: {err}")
                return

            job = resp["jobs"][0]
            status = job.get("status", None)
            status_reason = job.get("status_reason", None)
            log_stream_name = job["container"].get("logStreamName", None)
            params = {
                "logGroupName": log_group_name,
                "logStreamName": log_stream_name,
                "limit": 1000,
                "startFromHead": True,
            }

            if status == "SUCCEEDED" or status == "FAILED" or status == "CANCELLED":
                self.logger.warn(
                    f"job finished, status {status}, reason {status_reason}"
                )
                break

            if status == "RUNNING" and log_stream_name != None:
                args = (
                    {
                        **params,
                        "nextToken": next_token,
                    }
                    if next_token != ""
                    else params
                )

                try:
                    logs = logs_client.get_log_events(**args)
                except ClientError as err:
                    self.logger.exception(f"Error getting logs")

                next_token = logs["nextForwardToken"]
                events = logs["events"]
                for item in events:
                    message = item["message"]
                    self.logger.info(message)
            else:
                print(f"Status: {status}, Takes: {time.time() - start_time}", end="\r")

            time.sleep(1)
