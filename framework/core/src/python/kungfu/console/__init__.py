#  SPDX-License-Identifier: Apache-2.0

from kungfu.console import commands, variants

__ = [commands, variants, __builtins__]


def available():
    return reversed(__ and __.pop() and __) or [commands]


def select(modules, **kwargs):
    kwargs.pop("auto_envvar_prefix", None)
    next(m for m in modules if m.main(auto_envvar_prefix="KF", **kwargs))


from kungfu.serverless.store import FeatureStore

# fs = FeatureStore("alpha")
# print(fs.list_public_features())
# print(fs.get_public_feature_date_range("main-inflow-factor"))
# print(fs.get_public_feature_data("main-inflow-factor"))
