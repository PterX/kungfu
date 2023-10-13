const sdk = require('@kungfu-trader/kungfu-sdk');

module.exports = {
  flags: 'package',
  desc: 'Package kungfu extension',
  run: () => {
    if (sdk.lib.extension.checkIfSkipBuild()) {
      console.log('-- skip package on this platform');
      return;
    }
    sdk.lib.extension.package();
  },
};
