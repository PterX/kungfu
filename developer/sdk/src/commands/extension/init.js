const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'init',
  desc: 'Init extension project',
  run: () => {
    sdk.lib.extension.init();
  },
};
