const sdk = require('@kungfu-tech/sdk');

module.exports = {
  flags: 'cli',
  desc: 'Run kungfu artifact in cli mode',
  setup: (cli) => {
    cli.strict(false);
  },
  run: (argv) => {
    sdk.lib.craft.cli(argv);
  },
};
