from kungfu.yijinjing import time as kft

STEP_LIMIT = 0
MASTER_STEP_LIMIT = 10000


def run_forever(ctx, executor):
    if executor.get_home().name == "master":
        executor.run(MASTER_STEP_LIMIT)
    else:
        executor.run()
    executor.post_run()


def run_by_step(ctx, executor):
    ctx.logger.info(f"{executor.get_home_uid()} {executor.get_home_uname()} running")
    ctx.logger.debug(
        f"from {kft.strftime(executor.get_begin_time())} until {kft.strftime(executor.get_end_time())}"
    )
    executor.run(STEP_LIMIT)
    executor.post_run()
