const sdk = require('@kungfu-trader/kungfu-sdk');

module.exports = {
  flags: 'init',
  desc: 'init template',
  run: () => {
    sdk.lib.extension.init();
  },
};
