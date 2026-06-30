const { prebuilt } = require('@kungfu-tech/core');

module.exports = {
  flags: 'install',
  desc: 'Install kungfu prebuilt',
  run: () => {
    prebuilt('install');
  },
};
