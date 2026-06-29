const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'build',
  desc: 'Build kungfu operator',
  run: async () => {
    await sdk.lib.extension.installBatch();
    sdk.lib.extension.configure();
    sdk.lib.extension.compile();
  },
};
