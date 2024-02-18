PARAMS = {
  "backtest_job_definition_arn": {
    "sensitive": False,
    "type": "string",
    "value": "arn:aws-cn:batch:cn-north-1:186931371703:job-definition/serverless-kungfu-backtest-prod:8"
  },
  "backtest_job_queue_arn": {
    "sensitive": False,
    "type": "string",
    "value": "arn:aws-cn:batch:cn-north-1:186931371703:job-queue/serverless-kungfu-backtest-prod"
  },
  "backtest_log_group_arn": {
    "sensitive": False,
    "type": "string",
    "value": "arn:aws-cn:logs:cn-north-1:186931371703:log-group:/aws/batch/serverless-kungfu/backtest/prod"
  },
  "backtest_log_group_name": {
    "sensitive": False,
    "type": "string",
    "value": "/aws/batch/serverless-kungfu/backtest/prod"
  },
  "backtest_repository_url": {
    "sensitive": False,
    "type": "string",
    "value": "186931371703.dkr.ecr.cn-north-1.amazonaws.com.cn/serverless-kungfu-backtest-prod"
  },
  "backtest_s3_bucket_arn": {
    "sensitive": False,
    "type": "string",
    "value": "arn:aws-cn:s3:::serverless-kungfu-backtest-prod20231122020256155700000001"
  },
  "backtest_s3_bucket_name": {
    "sensitive": False,
    "type": "string",
    "value": "serverless-kungfu-backtest-prod20231122020256155700000001"
  },
  "identity_pool_id": {
    "sensitive": False,
    "type": "string",
    "value": "cn-north-1:02aaed1c-656e-46fa-aad2-5aa9600ebac4"
  },
  "oidc_provider_arn": {
    "sensitive": False,
    "type": "string",
    "value": "arn:aws-cn:iam::186931371703:oidc-provider/serverless-kungfu.authing.cn/oidc"
  },
  "region": {
    "sensitive": False,
    "type": "string",
    "value": "cn-north-1"
  }
}
