const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'upgrade',
  desc: 'Upgrade @kungfu-tech packages',
  run: () => {
    sdk.lib.craft.upgrade();
  },
};
