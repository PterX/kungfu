#!/usr/bin/env node

const { parseByCli } = require('./utils');

const commands = [
  'craft',
  'extension',
  'project',
  'strategy',
  'broker',
  'operator',
];

module.exports = require('@kungfu-tech/core').sywac(module, (cli) => {
  if (process.env.NODE_ENV === 'production') {
    commands.forEach((command) => {
      const opt = require(`./commands/${command}`);
      cli.command(opt);
    });

    parseByCli(cli, true);
  } else {
    cli.commandDirectory('commands');
  }
});
