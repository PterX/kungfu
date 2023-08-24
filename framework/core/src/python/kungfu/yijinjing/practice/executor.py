#  SPDX-License-Identifier: Apache-2.0

import importlib
import json
import os
import sys
import types
import kungfu
import glob
from pathlib import Path
from fnmatch import fnmatch

from kungfu.console import site
from kungfu.yijinjing import journal as kfj
from kungfu.yijinjing.log import find_logger
from kungfu.yijinjing import time as kft
from kungfu.yijinjing.practice.master import Master
from kungfu.yijinjing.practice.coloop import KungfuEventLoop
from kungfu.wingchun.strategy import Runner, Strategy
from kungfu.wingchun.sliceindexer import SliceIndexer
from kungfu.wingchun.report import Report
from kungfu.wingchun.operator import OpRunner, Operator

from collections import deque
from importlib.util import module_from_spec, spec_from_file_location
from os import path

lf = kungfu.__binding__.longfist
wc = kungfu.__binding__.wingchun
yjj = kungfu.__binding__.yijinjing


class ExecutorRegistry:
    def __init__(self, ctx):
        self.ctx = ctx
        self.executors = {
            "system": {"master": MasterLoader(ctx), "service": ServiceLoader(ctx)},
            "md": {},
            "td": {},
            "strategy": {"default": ExtensionLoader(self.ctx, None, None)},
            "operator": {},
        }

    def setup_log(self):
        ctx = self.ctx
        ctx.location = yjj.location(
            kfj.MODES[ctx.mode],
            kfj.CATEGORIES[ctx.category],
            ctx.group,
            ctx.name,
            ctx.backtest_locator
            if kfj.MODES[ctx.mode] == lf.enums.mode.BACKTEST
            else ctx.runtime_locator,
        )
        ctx.logger = find_logger(ctx.location, ctx.log_level)

    def load_extensions(self):
        self.setup_log()

        ctx = self.ctx
        ctx.logger.debug(f"finding kungfu extension for {ctx.location}")

        if ctx.extension_path:
            deque(map(self.register_extensions, ctx.extension_path.split(path.pathsep)))
        elif ctx.path:
            self.read_config(os.path.dirname(ctx.path))

        if ctx.group not in self.executors[ctx.category]:
            self.executors[ctx.category][ctx.group] = ExtensionLoader(ctx, None, None)

        if (
            ctx.category == "system"
            and ctx.group == "service"
            and ctx.name not in self.executors["system"]["service"]
        ):
            self.executors["system"]["service"].load_service(ctx)

    def register_extensions(self, root):
        for child in os.listdir(root):
            extension_dir = path.abspath(path.join(root, child))
            self.read_config(extension_dir)

    def read_config(self, extension_dir):
        config_path = os.path.join(extension_dir, "package.json")

        def report(reason):
            self.ctx.logger.debug(
                f"kungfu extension not found in {extension_dir}: {reason}"
            )

        if path.exists(config_path):
            with open(config_path, mode="r", encoding="utf8") as config_file:
                config = json.load(config_file)
                if "kungfuConfig" in config:
                    if "config" in config["kungfuConfig"]:
                        group = config["kungfuConfig"]["key"]
                        for category in config["kungfuConfig"]["config"]:
                            if category not in kfj.CATEGORIES:
                                raise RuntimeError(f"Unsupported category {category}")
                            if (
                                self.executors["strategy"]["default"]
                                and self.ctx.category == "strategy"
                                and self.ctx.group == "default"
                            ):
                                self.executors["strategy"]["default"].config = config
                            else:
                                self.executors[category][group] = ExtensionLoader(
                                    self.ctx, extension_dir, config
                                )
                    elif "key" in config["kungfuConfig"]:
                        if (
                            self.ctx.category == "strategy"
                            or self.ctx.category == "operator"
                        ):
                            group = config["kungfuConfig"]["key"]
                            self.executors[self.ctx.category][group] = ExtensionLoader(
                                self.ctx, extension_dir, config
                            )
                        else:
                            report("load extension config with unsupported category.")
                    else:
                        report("missing key/config in kungfuConfig")
                else:
                    report("missing kungfuConfig")

    def __getitem__(self, category):
        return self.executors[category]

    def __str__(self):
        return json.dumps(self.executors, indent=2, cls=RegistryJSONEncoder)

    def __repr__(self):
        return json.dumps(self.executors, cls=RegistryJSONEncoder)


class MasterLoader(dict):
    def __init__(self, ctx):
        super().__init__()
        self.ctx = ctx
        self["master"] = self.run

    def run(self, mode: str, low_latency: bool):
        self.ctx.location = yjj.location(
            kfj.MODES[mode],
            lf.enums.category.SYSTEM,
            "master",
            "master",
            self.ctx.runtime_locator,
        )
        self.ctx.logger = find_logger(self.ctx.location, self.ctx.log_level)
        Master(self.ctx).run()


class ServiceLoader(dict):
    def __init__(self, ctx):
        super().__init__()
        self.ctx = ctx
        self["ledger"] = self.create_service("ledger", wc.Ledger)

    def create_service(self, name, service):
        def run(mode: str, low_latency: bool):
            self.ctx.location = yjj.location(
                kfj.MODES[mode],
                lf.enums.category.SYSTEM,
                "service",
                name,
                self.ctx.runtime_locator,
            )
            self.ctx.logger = find_logger(self.ctx.location, self.ctx.log_level)
            self.ctx.logger.info(
                f"starting service {name}, low_latency={low_latency}, arguments={self.ctx.arguments}"
            )
            if "is_python_service" in dir(service) and service.is_python_service:
                service(self.ctx).run()
            else:
                service(
                    self.ctx.runtime_locator,
                    kfj.MODES[self.ctx.mode],
                    self.ctx.low_latency,
                    self.ctx.arguments,
                ).run()

        return run

    def load_service(self, ctx):
        sys.path.append(ctx.extension_path)
        module = importlib.import_module(ctx.name)
        service_builder = getattr(module, "service")
        self[ctx.name] = self.create_service(ctx.name, service_builder)


class ExtensionLoader:
    def __init__(self, ctx, extension_dir, config):
        self.ctx = ctx
        self.extension_dir = extension_dir
        self.config = config

    def __getitem__(self, name):
        return ExtensionExecutor(self.ctx, self)

    def __str__(self):
        return self.config["kungfuConfig"]["name"]

    def __repr__(self):
        return self.__str__()


class ExtensionExecutor:
    def __init__(self, ctx, loader):
        self.ctx = ctx
        self.loader = loader
        self.runners = {
            "md": self.run_market_data,
            "td": self.run_trader,
            "strategy": self.run_strategy,
            "operator": self.run_operator,
        }

    def __call__(self, mode, low_latency):
        self.runners[self.ctx.category]()

    def setup(self, loader, use_ctx_path=True):
        if loader.extension_dir:
            site.setup(loader.extension_dir)
            sys.path.insert(0, loader.extension_dir)
        elif use_ctx_path:
            dirname = os.path.dirname(self.ctx.path)
            site.setup(dirname)
            sys.path.insert(0, dirname)

    def run_broker_vendor(self, vendor_builder):
        ctx = self.ctx
        loader = self.loader
        location = yjj.location(
            kfj.MODES[ctx.mode],
            kfj.CATEGORIES[ctx.category],
            ctx.group,
            ctx.name,
            ctx.runtime_locator,
        )

        self.setup(loader, use_ctx_path=False)
        # let TD and MD start without package.json
        sys.path.insert(0, ctx.extension_path)
        module = importlib.import_module(ctx.group)
        self.ctx.logger.info(f"loading {ctx.group} from {loader.extension_dir}")
        vendor = vendor_builder(
            ctx.runtime_locator, ctx.group, ctx.name, ctx.low_latency, ctx.arguments
        )
        service_builder = getattr(module, ctx.category)
        self.ctx.logger.debug(f"loaded service builder")
        service = service_builder(vendor)
        self.ctx.logger.debug("set service for vendor")
        vendor.set_service(service)
        self.ctx.logger.info(f"vendor {location.uname} ready to run")
        vendor.run()

    def run_market_data(self):
        self.run_broker_vendor(wc.MarketDataVendor)

    def run_trader(self):
        self.run_broker_vendor(wc.TraderVendor)

    def run_strategy(self):
        loader = self.loader
        self.setup(loader, use_ctx_path=True)
        ctx = self.ctx
        locator = (
            ctx.backtest_locator
            if kfj.MODES[ctx.mode] == lf.enums.mode.BACKTEST
            else ctx.runtime_locator
        )
        ctx.location = yjj.location(
            kfj.MODES[ctx.mode],
            lf.enums.category.STRATEGY,
            ctx.group,
            ctx.name,
            locator,
        )
        os.environ["KF_STG_GROUP"] = ctx.group
        os.environ["KF_STG_NAME"] = ctx.name
        ctx.runner = load_runner(ctx, locator)  # 先load runner才能识别出定制的Strategy
        if loader.config is None:
            load = False
            json_config = os.path.join(os.path.dirname(ctx.path), "package.json")
            # 如果策略目录下有package.json, 则从package.json里面读取key值作为策略的python模块名
            if path.exists(json_config):
                with open(json_config, mode="r", encoding="utf8") as json_config_out:
                    config = json.load(json_config_out)
                    if "kungfuConfig" in config and "key" in config["kungfuConfig"]:
                        key = config["kungfuConfig"]["key"]
                        load = True
                        ctx.strategy = load_module(ctx, ctx.path, key, Strategy)
            # 如果没有从策略目录下的读取到package.json, 则用ctx.group作为key值去导入策略模块, ctx.group是策略的python模块名
            if not load:
                ctx.strategy = load_module(ctx, ctx.path, ctx.group, Strategy)
        else:
            ctx.strategy = load_module(
                ctx, ctx.path, loader.config["kungfuConfig"]["key"], Strategy
            )

        if kfj.MODES[ctx.mode] == lf.enums.mode.BACKTEST:
            matcher = load_matcher(ctx, ctx.matcher)
            if matcher:
                ctx.runner.set_matcher(matcher)
            begin_time_stamp, end_time_stamp = self.parse_begin_end(ctx)
            ctx.runner.set_begin_time(begin_time_stamp)
            ctx.runner.set_end_time(end_time_stamp)
            from_indexer, to_indexer = self.parse_from_to_indexer(
                ctx, begin_time_stamp, end_time_stamp
            )
            ctx.runner.set_from_indexer(from_indexer)
            ctx.runner.set_to_indexer(to_indexer)
            if ctx.report:
                report = load_report(ctx, ctx.report)
                ctx.runner.set_report(report)
        if kfj.MODES[ctx.mode] == lf.enums.mode.REPLAY:
            begin_time_stamp, end_time_stamp = self.parse_begin_end(ctx)
            ctx.runner.set_begin_time(begin_time_stamp)
            ctx.runner.set_end_time(end_time_stamp)

        ctx.runner.add_strategy(ctx.strategy)

        if kfj.MODES[ctx.mode] == lf.enums.mode.LIVE and "is_cpp_module" not in dir(
            ctx
        ):
            ctx.logger.info("use run_forever")
            ctx.loop = KungfuEventLoop(ctx, ctx.runner)
            ctx.loop.run_forever()
        else:
            ctx.logger.info("use run")
            ctx.runner.run()
        if kfj.MODES[ctx.mode] == lf.enums.mode.BACKTEST and ctx.report:
            report.sumerize()

    def run_operator(self):
        loader = self.loader
        self.setup(loader, use_ctx_path=True)
        ctx = self.ctx
        ctx.location = yjj.location(
            kfj.MODES[ctx.mode],
            lf.enums.category.OPERATOR,
            ctx.group,
            ctx.name,
            ctx.runtime_locator,
        )
        os.environ["KF_OP_GROUP"] = ctx.group
        # TODO check extension.h for implementation details, how to deal with 1 runner : N operators?
        os.environ["KF_OP_NAME"] = ctx.name
        if ctx.path is None:
            module_path = list(
                filter(
                    lambda file_name: fnmatch(file_name, "*.so")
                    or fnmatch(file_name, "*.pyd")
                    or fnmatch(file_name, "*.py"),
                    glob.glob(os.path.join(loader.extension_dir, ctx.group + "*")),
                )
            )[0]
            ctx.path = os.path.abspath(module_path)
        if loader.config is None:
            load = False
            json_config = os.path.join(os.path.dirname(ctx.path), "package.json")
            if path.exists(json_config):
                with open(json_config, mode="r", encoding="utf8") as json_config_out:
                    config = json.load(json_config_out)
                    if "kungfuConfig" in config and "key" in config["kungfuConfig"]:
                        key = config["kungfuConfig"]["key"]
                        load = True
                        ctx.operator = load_module(ctx, ctx.path, key, Operator)
            if not load:
                ctx.operator = load_module(ctx, ctx.path, ctx.group, Operator)
        else:
            ctx.operator = load_module(
                ctx, ctx.path, loader.config["kungfuConfig"]["key"], Operator
            )
        ctx.op_runner = OpRunner(ctx, kfj.MODES[ctx.mode])
        if kfj.MODES[ctx.mode] == lf.enums.mode.BACKTEST:
            begin_time_stamp, end_time_stamp = self.parse_begin_end(ctx)
            ctx.op_runner.set_begin_time(begin_time_stamp)
            ctx.op_runner.set_end_time(end_time_stamp)
            from_indexer, to_indexer = self.parse_from_to_indexer(
                ctx, begin_time_stamp, end_time_stamp
            )
            ctx.op_runner.set_from_indexer(from_indexer)
            ctx.op_runner.set_to_indexer(to_indexer)
            if ctx.report:
                report = load_report(ctx, ctx.report)
                ctx.op_runner.set_report(report)
        # ctx.runner = self.load_runner(ctx)
        if kfj.MODES[ctx.mode] == lf.enums.mode.REPLAY:
            begin_time_stamp, end_time_stamp = self.parse_begin_end(ctx)
            ctx.op_runner.set_begin_time(begin_time_stamp)
            ctx.op_runner.set_end_time(end_time_stamp)

        ctx.op_runner.add_operator(ctx.operator)
        ctx.op_runner.run()
        if kfj.MODES[ctx.mode] == lf.enums.mode.BACKTEST and ctx.report:
            report.sumerize()

    def parse_begin_end(self, ctx):
        ctx.logger.debug(f"ctx.mode: {ctx.mode}")

        if kfj.MODES[ctx.mode] == lf.enums.mode.BACKTEST and (
            not ctx.begin or not ctx.end
        ):
            raise ValueError("backtest mode must specify begin and end")

        if kfj.MODES[ctx.mode] == lf.enums.mode.REPLAY and (
            not (ctx.begin and ctx.end) and not ctx.session_id
        ):
            raise ValueError("replay mode must specify begin and end or session_id")

        begin_time_stamp = (
            kft.strptimes(
                ctx.begin,
                ("%F %T", "%F %T.%N", "%Y%m%d", "%Y-%m-%d", "%Y-%m-%d %H:%M:%S"),
            )
            if ctx.begin
            else yjj.now_in_nano()
        )
        end_time_stamp = (
            kft.strptimes(
                ctx.end,
                ("%F %T", "%F %T.%N", "%Y%m%d", "%Y-%m-%d", "%Y-%m-%d %H:%M:%S"),
            )
            if ctx.end
            else yjj.now_in_nano()
        )

        if ctx.session_id:
            session = kfj.find_session(ctx, ctx.session_id)
            begin_time_stamp = session["begin_time"]
            end_time_stamp = min(
                (session["end_time"] if session.closed else yjj.now_in_nano()),
                end_time_stamp,
            )

        ctx.logger.debug(
            f"begin time: {kft.strftime(begin_time_stamp)}, end_time_stamp: {kft.strftime(end_time_stamp)}"
        )
        return begin_time_stamp, end_time_stamp

    def parse_from_to_indexer(self, ctx, begin, end):
        from_indexer = wc.SliceIndexer(begin, end)
        to_indexer = wc.SliceIndexer(begin, end)
        if ctx.from_indexer:
            from_indexer = SliceIndexer(ctx, begin, end, ctx.from_indexer)
            # from_indexer = wc.DayIndexer(begin, end)
        if ctx.to_indexer:
            to_indexer = SliceIndexer(ctx, begin, end, ctx.to_indexer)
        return from_indexer, to_indexer


class RegistryJSONEncoder(json.JSONEncoder):
    def default(self, obj):
        test = isinstance(obj, ExtensionLoader) or isinstance(obj, types.FunctionType)
        return str(obj) if test else obj.__dict__


def load_module(ctx, path, key, cls):
    cls_name = cls.__name__
    ctx.logger.debug(f"loading {cls_name} from {path}")
    ctx.logger.debug(f"{cls_name} key: {key}")
    ctx.logger.debug(f"{cls_name} dirname: {os.path.dirname(path)}")

    if path.endswith(".py"):
        return cls(ctx)  # keep strategy alive for pybind11
    elif key is not None and (path.endswith(".so") or path.endswith(".pyd")):
        return try_load_cpp_module(ctx, path, key, cls)
    elif key is not None and path.endswith(key):
        return cls(ctx)
    else:
        ctx.path = os.path.join(os.path.dirname(path), key)
        return cls(ctx)


def load_matcher(ctx, path):
    if not ctx.matcher:
        return None
    try:
        sys.path.append(str(Path(path).parent))
        lib_name = Path(path).stem.split(".")[0]
        module = importlib.import_module(lib_name)
        ctx.logger.debug(f"import matcher: {lib_name} success")
        matcher_builder = getattr(module, "matcher")
        return matcher_builder()
    except Exception as e:
        ctx.logger.debug("load_matcher failed: {}".format(e))
        ctx.logger.warn("matcher path: {} cannot be import by python".format(path))
        raise e


def load_report(ctx, path):
    cls = Report
    try:
        if path.endswith(".py") or os.path.isdir(path):
            return cls(ctx)  # keep strategy alive for pybind11
        elif path.endswith(".so") or path.endswith(".pyd"):
            sys.path.append(str(Path(path).parent))
            lib_name = Path(path).stem.split(".")[0]
            try:
                module = importlib.import_module(lib_name)
                ctx.logger.debug(f"import as cpp {lib_name} success")
                factory_func = getattr(module, cls.__name__.lower())
                return factory_func()
            except Exception as e:
                ctx.logger.debug(f"fallback to python loader due to: {e}")
                ctx.report = os.path.join(os.path.dirname(path), lib_name)
                return cls(ctx)
        raise FileNotFoundError(f"report path: {path} not found")
    except Exception as e:
        ctx.logger.debug("load_report failed: {}".format(e))
        ctx.logger.critical("report path: {} cannot be imported.".format(path))
        raise e


def try_load_cpp_module(ctx, path, key, cls):
    cls_name = cls.__name__
    try:
        module = importlib.import_module(key)
        ctx.logger.debug(f"import as cpp {cls_name} success")
        factory_func = getattr(module, cls_name.lower())
        ctx.is_cpp_module = True
        return factory_func()
    except Exception as e:
        ctx.logger.debug(f"fallback to python loader due to: {e}")
        ctx.path = os.path.join(os.path.dirname(path), key)
        return cls(ctx)


def load_runner(ctx, locator):
    if ctx.vendor is not None:
        sys.path.append(ctx.extension_path)
        module = importlib.import_module(ctx.vendor)
        runner_vendor = getattr(module, "Runner")
        runner = runner_vendor(
            locator,
            ctx.group,
            ctx.name,
            kfj.MODES[ctx.mode],
            ctx.low_latency,
            ctx.arguments,
        )
        return runner
    else:
        return Runner(ctx, locator, kfj.MODES[ctx.mode])
