from kungfu.yijinjing import time as kft

STEP_LIMIT = 0
MASTER_STEP_LIMIT = 10000


def run_forever(ctx, executor):
    if executor.get_home().name == "master":
        executor.run(MASTER_STEP_LIMIT)
    else:
        executor.run(STEP_LIMIT)
    executor.post_run()
