const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'format',
  desc: 'Format kungfu extension code',
  run: () => {
    sdk.lib.extension.format();
  },
};
