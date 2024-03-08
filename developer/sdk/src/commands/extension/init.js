const sdk = require('@kungfu-trader/kungfu-sdk');

module.exports = {
  flags: 'init',
  desc: 'Init template',
  run: () => {
    sdk.lib.extension.init();
  },
};
