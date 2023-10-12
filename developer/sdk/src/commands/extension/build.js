const sdk = require('@kungfu-trader/kungfu-sdk');

module.exports = {
  flags: 'build',
  desc: 'Build kungfu extension',
  setup: (cli) => {
    cli
      .option('url', {
        type: 'string',
        desc: 'lib site URL',
        defaultValue: sdk.lib.extension.DefaultLibSiteURL,
      })
      .option('update-project-config', {
        type: 'boolean',
        desc: 'update project config files',
        defaultValue: true,
      });
  },
  run: async (argv) => {
    if (sdk.lib.extension.checkIfSkipBuild()) {
      console.log('-- skip build on this platform');
      return;
    }
    if (argv['update-project-config']) {
      sdk.lib.project.configure(false, true);
    }
    await sdk.lib.extension.installBatch(argv.url);
    sdk.lib.extension.configure();
    sdk.lib.extension.compile();
  },
};
