const sdk = require('@kungfu-trader/kungfu-sdk');

module.exports = {
  flags: 'init',
  desc: 'Init extension project',
  run: () => {
    sdk.lib.extension.init();
  },
};
