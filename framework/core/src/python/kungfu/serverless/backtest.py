import boto3
import os
import json
import kungfu
import requests
import time
import signal
from kungfu.serverless.sso import SSO
from kungfu.serverless.utils import (
    get_credentials_for_identity,
    read_file_content,
    get_tokens,
)

yjj = kungfu.__binding__.yijinjing


class Backtest:
    def __init__(self, stage="prod"):
        self.stage = stage
        self.sso = SSO(stage)

        if self.sso.introspect_token() != True:
            print("Please Login First, Try kfc login")
            raise Exception("Login Required, Try kfc login")

        self.sso.get_new_access_token_by_refresh_token()
        phone, username, user_id = self.sso.get_profile()
        print(f"Backtest init successfully, phone {phone} username {username}")
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
        self.__put_resource(
            file_path, parameter_map, access_key, secret_key, session_token
        )
        job_id = self.__run_job(file_path, begin_time, end_time, level, parameter_map)
        log_group_name = parameter_map[self.LOG_GROUP_PARAM_NAME]     
        self.__monit_log(log_group_name, job_id, access_key, secret_key, session_token)
    

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
        resp = client.get_parameters(
            Names=[
                self.JOB_DEFINITION_ARN_PARAM_NAME,
                self.JOB_QUEUE_ARN_PARAM_NAME,
                self.S3_BUCKET_PARAM_NAME,
                self.LOG_GROUP_PARAM_NAME,
            ]
        )
        parameters = resp["Parameters"]
        parameter_map = {}
        for item in parameters:
            parameter_map[item["Name"]] = item["Value"]

        return parameter_map

    def __put_resource(
        self, file_path, parameter_map, access_key, secret_key, session_token
    ):
        file_basename = os.path.basename(file_path)
        file_content = read_file_content(file_path)

        objectKey = f"{self.user_id}/upload/{file_basename}"
        s3 = boto3.client(
            "s3",
            region_name="cn-north-1",
            aws_access_key_id=access_key,
            aws_secret_access_key=secret_key,
            aws_session_token=session_token,
        )

        bucket_name = parameter_map[self.S3_BUCKET_PARAM_NAME]
        s3.put_object(Bucket=bucket_name, Key=objectKey, Body=file_content)

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

        access_token, refresh_token, id_token = get_tokens()
        headers = {
            "Content-Type": "application/json",
            "Authorization": id_token,
        }
        resp = requests.post(
            f"https://api.kungfu-trader.com/{self.stage}/backtest/submitjob",
            data=json.dumps(data),
            headers=headers,
        ).text
        resp = json.loads(resp)
        jobId = resp.get("jobId", None)
        if not jobId:
            print(resp.text["message"])
            raise Exception("Job sumbitted failed")

        print(f"Job sumbitted, id: {jobId}")
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
            batch_client.terminate_job(jobId=job_id, reason="user triggered")
            exit()
                        
        signal.signal(signal.SIGINT, try_exit)
        signal.signal(signal.SIGTERM, try_exit)
        
        next_token = ""
        start_time = time.time()
        while True:
            resp = batch_client.describe_jobs(jobs=[job_id])
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
                print(f"job finished, status {status}, reason {status_reason}")
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
                logs = logs_client.get_log_events(**args)
                next_token = logs["nextForwardToken"]
                events = logs["events"]
                for item in events:
                    message = item["message"]
                    print(message)
            else:
                print(f"Status: {status}, Takes: {time.time() - start_time}", end="\r")

            time.sleep(1)