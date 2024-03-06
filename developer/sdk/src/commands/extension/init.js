const sdk = require('@kungfu-trader/kungfu-sdk');

module.exports = {
  flags: 'Init',
  desc: 'Init template',
  run: () => {
    sdk.lib.extension.init();
  },
};
