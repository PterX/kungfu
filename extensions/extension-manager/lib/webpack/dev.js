'use strict';

const path = require('path');
const { shell, utils } = require('@kungfu-tech/api/toolkit');

function start(argv, name) {
  return new Promise((resolve) => {
    const config = require(`../webpack/webpack${
      name ? '.' + name : ''
    }.config`)(argv);
    const compiler = require('webpack')(config);
    compiler.watch({}, (err, stats) => {
      if (err) {
        console.log(err);
        return;
      }

      shell.logStats(name, stats);
    });

    compiler.hooks.done.tap('components-compile-done', (stats) => {
      shell.logStats(name, stats);
      resolve();
    });
  });
}

const run = (distDir, distName = 'extensionManager') => {
  shell.greeting();

  const argv = utils.buildDevArgv(distDir, distName);
  process.chdir(path.join(distDir, '..'));

  return Promise.all([start(argv, '')]);
};

module.exports = run;

if (require.main === module) {
  const distDir = path.join(process.cwd().toString(), 'dist');
  shell.runDist(distDir, run);
}
