const fse = require('fs-extra');
const path = require('path');
const { spawnSync } = require('child_process');
const { shell } = require('@kungfu-tech/core');
const { customResolve } = require('../utils');
const {
  getAppDir,
  getCliDir,
  getSdkDir,
  getJsApi,
} = require('@kungfu-tech/api/toolkit/utils');

const ensureDir = (cwd, ...dirNames) => {
  const targetDir = path.join(cwd, ...dirNames);
  fse.mkdirSync(targetDir, { recursive: true });
  return targetDir;
};

exports.build = () => {
  const appDistDir = path.join(getAppDir(), 'dist', 'app');
  const publicDir = path.join(getAppDir(), 'public');
  const cliDistDir = path.join(getCliDir(), 'dist', 'cli');
  const apiDistDir = path.join(getJsApi(), 'dist', 'api');
  const kfsDistDir = path.join(getSdkDir(), 'dist', 'sdk');

  const targetDistDir = ensureDir(process.cwd().toString(), 'dist');
  const targetAppDistDir = ensureDir(targetDistDir, 'app');
  const targetPublicDistDir = ensureDir(targetDistDir, 'public');
  const targetCliDistDir = ensureDir(targetDistDir, 'cli');
  const targetApiDistDir = ensureDir(targetDistDir, 'api');
  const targetKfsDistDir = ensureDir(targetDistDir, 'kfs');
  const targetCliDistPublicDir = ensureDir(getCliDir(), 'dist', 'public');

  shell.verifyElectron();

  fse.removeSync(targetDistDir);
  fse.copySync(appDistDir, targetAppDistDir, {});
  fse.copySync(publicDir, targetPublicDistDir, {});
  fse.copySync(cliDistDir, targetCliDistDir, {});
  fse.copySync(apiDistDir, targetApiDistDir, {});
  fse.copySync(kfsDistDir, targetKfsDistDir, {});
  fse.copySync(publicDir, targetCliDistPublicDir, {});
};

exports.package = async () => {
  const buildDir = ensureDir(process.cwd().toString(), 'build');
  await require('@kungfu-tech/gui').electronBuild(buildDir);
};

exports.dev = async (withWebpack) => {
  shell.verifyElectron();
  try {
    await require('@kungfu-tech/gui').devRun(
      ensureDir(process.cwd().toString(), 'dist'),
      'app',
      withWebpack,
    );
  } catch (err) {
    console.warn(err);
  }
};

exports.cli = () => {
  const cliPath = customResolve('@kungfu-tech/tui');
  const runExecutable = path.join(cliPath, '..', 'dev', 'cli.dev.js');
  spawnSync('node', [runExecutable, ...process.argv.slice(4)], {
    stdio: 'inherit',
    windowsHide: true,
  });
};

exports.upgrade = () => {
  shell.run('pnpm', ['update', '@kungfu-tech/*']);
};
