const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'configure',
  desc: 'Configure kungfu extensions',
  run: () => {
    sdk.lib.extension.configure();
  },
};
