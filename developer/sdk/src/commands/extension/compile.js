const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'compile',
  desc: 'Compile kungfu extensions',
  run: () => {
    sdk.lib.extension.compile();
  },
};
