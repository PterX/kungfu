const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'package',
  desc: 'Package kungfu strategy',
  run: () => {
    sdk.lib.extension.package();
  },
};
