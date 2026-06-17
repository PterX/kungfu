// SPDX-License-Identifier: Apache-2.0

const fse = require('fs-extra');
const path = require('path');
const { shell } = require('../lib');

function conan(cmd) {
  const pipenv_args = ['run', 'conan', ...cmd];
  shell.run('pipenv', pipenv_args, true, {
    env: { NODE_GYP_RUN: 'on', ...process.env },
  });
}

function getNodeVersionOptions() {
  const packageJson = fse.readJsonSync(
    path.resolve(path.dirname(__dirname), 'package.json'),
  );
  // electron 从 devDependencies 读并去掉 ^/~ 前缀；node_version 从 config 读
  // (v4 起 @kungfu-trader/libnode 不再列为 devDep，dev 走 npm link，见 .v4/docs/conan2-migration.md)。
  const electronVersion = String(packageJson.devDependencies['electron']).replace(
    /^[\^~]/,
    '',
  );
  const nodeVersion = packageJson.config['node_version'];
  return [
    '-o',
    `electron_version=${electronVersion}`,
    '-o',
    `node_version=${nodeVersion}`,
    '-o',
    'with_yarn=True',
  ];
}

function makeConanSetting(name) {
  return ['-s', `${name}=${shell.getConfigValue(name)}`];
}

function makeConanSettings(names) {
  return names.map(makeConanSetting).flat();
}

function makeConanOption(name) {
  return ['-o', `${name}=${shell.getConfigValue(name)}`];
}

function makeConanOptions(names) {
  return names.map(makeConanOption).flat().concat(getNodeVersionOptions());
}

// conan2：-if/-bf → --output-folder；arch 是 setting 由 profile 自测，不再作 -o 选项。
function conanInstall() {
  const settings = makeConanSettings(['build_type']);
  const options = makeConanOptions(['log_level', 'freezer']);
  conan([
    'install',
    '.',
    '--output-folder',
    'build',
    '--build=missing',
    ...settings,
    ...options,
  ]);
}

function conanBuild() {
  const settings = makeConanSettings(['build_type']);
  const options = makeConanOptions(['log_level', 'freezer']);
  conan(['build', '.', '--output-folder', 'build', ...settings, ...options]);
}

// conan2 移除了独立的 `conan package` 本地命令(package() 经 conan create/export-pkg 触发)。
// freeze→kfc 打包到 dist/kfc 的流程属 Stage C，需用 conan2 export-pkg/重构,见
// .v4/docs/conan2-migration.md。此处暂保留 conan build 触发(build() 仅编译,不 freeze)，
// freeze 入口在 Stage C 单独接通。
function conanPackage() {
  const settings = makeConanSettings(['build_type']);
  const options = makeConanOptions(['log_level', 'freezer']);
  conan(['build', '.', '--output-folder', 'build', ...settings, ...options]);
}

const cli = require('sywac')
  .command('install', {
    run: conanInstall,
  })
  .command('build', {
    run: conanBuild,
  })
  .command('package', {
    run: conanPackage,
  })
  .help('--help')
  .version('--version')
  .showHelpByDefault();

async function main() {
  await cli.parseAndExit();
}

module.exports.cli = cli;
module.exports.main = main;

if (require.main === module) main().catch(shell.utils.exitOnError);
