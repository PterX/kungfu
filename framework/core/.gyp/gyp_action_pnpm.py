#  SPDX-License-Identifier: Apache-2.0

from gyp_action_lib import Shell

Shell.run(["pnpm", "run", *Shell.extract_argv()])
