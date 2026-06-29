if (!process.env.CI) {
  const { shell } = require('./framework/core');
  const opts = { silent: true };
  shell.run('pnpm', ['run', 'sync'], true, opts);
  shell.run('pnpm', ['run', 'format'], true, opts);
}
