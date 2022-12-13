const fse = require('fs-extra');
const path = require('path');
const { spawnSync } = require('child_process');
const { shell } = require('@kungfu-trader/kungfu-core');
const { customResolve } = require('../utils');

const ensureDir = (cwd, ...dirNames) => {
  const targetDir = path.join(cwd, ...dirNames);
  fse.mkdirSync(targetDir, { recursive: true });
  return targetDir;
};

exports.build = () => {
  const {
    getAppDir,
    getCliDir,
    getSdkDir,
  } = require('@kungfu-trader/kungfu-js-api/toolkit/utils');

  const appDistDir = path.join(getAppDir(), 'dist', 'app');
  const publicDir = path.join(getAppDir(), 'public');
  const cliDistDir = path.join(getCliDir(), 'dist', 'cli');
  const kfsDistDir = path.join(getSdkDir(), 'dist', 'sdk');

  const targetDistDir = ensureDir(process.cwd().toString(), 'dist');
  const targetAppDistDir = ensureDir(targetDistDir, 'app');
  const targetPublicDistDir = ensureDir(targetDistDir, 'public');
  const targetCliDistDir = ensureDir(targetDistDir, 'cli');
  const targetKfsDistDir = ensureDir(targetDistDir, 'kfs');
  const targetCliDistPublicDir = ensureDir(getCliDir(), 'dist', 'public');

  shell.verifyElectron();

  fse.removeSync(targetDistDir);
  fse.copySync(appDistDir, targetAppDistDir, {});
  fse.copySync(publicDir, targetPublicDistDir, {});
  fse.copySync(cliDistDir, targetCliDistDir, {});
  fse.copySync(kfsDistDir, targetKfsDistDir, {});
  fse.copySync(publicDir, targetCliDistPublicDir, {});
};

exports.package = async () => {
  const buildDir = ensureDir(process.cwd().toString(), 'build');
  try {
    await require('@kungfu-trader/kungfu-app').electronBuild(buildDir);
  } catch (err) {
    console.warn(err);
  }
};

exports.dev = async (withWebpack) => {
  shell.verifyElectron();
  try {
    await require('@kungfu-trader/kungfu-app').devRun(
      ensureDir(process.cwd().toString(), 'dist'),
      'app',
      withWebpack,
    );
  } catch (err) {
    console.warn(err);
  }
};

exports.cli = () => {
  const cliPath = customResolve('@kungfu-trader/kungfu-cli');
  const runExecutable = path.join(cliPath, '..', 'dev', 'cli.dev.js');
  spawnSync('node', [runExecutable, ...process.argv.slice(4)], {
    stdio: 'inherit',
    windowsHide: true,
  });
};

exports.upgrade = () => {
  shell.run('yarn upgrade', ['--scope', '@kungfu-trader']);
};
