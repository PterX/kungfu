const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'package',
  desc: 'Package kungfu prebuilt',
  run: () => {
    sdk.lib.project.package();
  },
};
