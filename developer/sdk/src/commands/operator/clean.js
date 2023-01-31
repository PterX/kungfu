const sdk = require('@kungfu-trader/kungfu-sdk');

module.exports = {
  flags: 'clean',
  desc: 'Clean build operator files',
  run: () => {
    sdk.lib.extension.clean();
  },
};
