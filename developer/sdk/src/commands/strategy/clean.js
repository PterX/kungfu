const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'clean',
  desc: 'Clean build files',
  run: () => {
    sdk.lib.extension.clean();
  },
};
