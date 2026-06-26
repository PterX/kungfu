// SPDX-License-Identifier: Apache-2.0

const fse = require('fs-extra');
const path = require('path');
const { spawnSync } = require('child_process');

const buildType = process.env.npm_package_config_build_type;
const srcDir = path.join('src', 'python');
const buildDir = path.join('build', buildType);
const wheelDir = path.join('build', 'python');
const vsDir = path.join('.deps', 'vs');

fse.removeSync(wheelDir);
fse.copySync(srcDir, wheelDir, {
  filter: (p) => !path.basename(p).startsWith('kfc.'),
});
fse.copySync(buildDir, wheelDir, {
  filter: (p) => !path.basename(p).endsWith('.node'),
});
if (process.platform == 'win32') {
  fse.copySync(vsDir, wheelDir);
}

// uv 接管 env（S1 阶段 A）：从 build/python 子目录运行，uv 向上发现 framework/core 的
// pyproject + .venv；--frozen 不偷改 uv.lock。setup.py 仍走 poetry-core(阶段 B 再脱)。
const uv_args = ['run', '--frozen', 'python', 'setup.py', 'bdist_wheel'];

console.log(`$ uv ${uv_args.join(' ')}`);

const result = spawnSync('uv', uv_args, {
  shell: true,
  stdio: 'inherit',
  windowsHide: true,
  cwd: wheelDir,
});

process.exit(result.status);
