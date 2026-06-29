const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'build',
  desc: 'Build kungfu artifact',
  run: () => {
    sdk.lib.craft.build();
  },
};
