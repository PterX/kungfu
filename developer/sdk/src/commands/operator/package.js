const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'package',
  desc: 'Package kungfu operator',
  run: () => {
    sdk.lib.extension.package();
  },
};
