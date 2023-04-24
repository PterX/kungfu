const sdk = require('@kungfu-trader/kungfu-sdk');

module.exports = {
  flags: 'package',
  desc: 'Package kungfu operator',
  run: () => {
    sdk.lib.extension.package();
  },
};
