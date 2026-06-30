const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'package',
  desc: 'Package kungfu artifact',
  run: async () => {
    await sdk.lib.craft.package();
  },
};
