const { parseByCli } = require('../utils');

const subcommands = ['build', 'clean', 'package'];

module.exports = {
  aliases: 's',
  flags: 'operator <subcommand>',
  ignore: '<subcommand>',
  desc: 'Build Kungfu Operator',
  setup: (cli) => {
    if (process.env.NODE_ENV === 'production') {
      subcommands.forEach((subcommand) => {
        const opt = require(`./operator/${subcommand}`);
        cli.command(opt);
      });
    } else {
      cli.commandDirectory('operator');
    }

    parseByCli(cli);
  },
};
