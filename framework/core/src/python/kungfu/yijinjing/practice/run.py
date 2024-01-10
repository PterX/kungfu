from kungfu.yijinjing import time as kft

STEP_LIMIT = 1000


def run_forever(ctx, executor):
    executor.run()
    executor.post_run()


def run_by_step(ctx, executor):
    ctx.logger.info(f"{executor.get_home_uid()} {executor.get_home_uname()} running")
    ctx.logger.debug(
        f"from {kft.strftime(executor.get_begin_time())} until {kft.strftime(executor.get_end_time())}"
    )
    executor.pre_setup()
    executor.setup()
    ctx.logger.debug("app setup done")
    while executor.is_live():
        executor.step(STEP_LIMIT)
    executor.on_exit()
    executor.post_run()
