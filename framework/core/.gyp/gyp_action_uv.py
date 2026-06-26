#  SPDX-License-Identifier: Apache-2.0
#
# uv 接管 Python env（S1 阶段 A 接入，goal 2026-06-26-kungfu-buildchain-modernization）。
# 用一步 `uv sync --frozen` 取代旧 pipenv(seed) + poetry(install) 两步：
# - --frozen 严格按已提交 uv.lock 安装，构建期不偷改 lock，保证可复现。
# - pyproject 改了但没 `uv lock` 时 --frozen 会报错，属预期：提示先刷新 lock，不在构建里静默漂移。
# 旧 gyp_action_pipenv.py / gyp_action_poetry.py 保留在盘上作回退（双轨期；绿了再退役）。

from gyp_action_lib import Shell

Shell.run(["uv", "sync", "--frozen"])
Shell.touch("uv.lock")
