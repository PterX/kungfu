const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'clean',
  desc: 'Clean build operator files',
  run: () => {
    sdk.lib.extension.clean();
  },
};
