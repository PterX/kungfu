const { getCliDir } = require('@kungfu-tech/api/toolkit/utils');
const path = require('path');
const cliDir = getCliDir();

module.exports = {
  devRun: require('./dev/run.js'),
  webpackBuild: require('./webpack/build.js'),
  defaultDistDir: path.join(cliDir, 'dist'),
};
