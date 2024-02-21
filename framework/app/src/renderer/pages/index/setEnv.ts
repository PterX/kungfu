// ELECTRON_RUN_AS_NODE 应用于通过process.execPath开启任务
process.env.ELECTRON_RUN_AS_NODE = true;
process.env.ELECTRON_ENABLE_STACK_DUMPING = true;
process.env.BY_PASS_RESTORE = true;
process.env.APP_ID = 'app';
process.env.RELOAD_AFTER_CRASHED = process.argv.includes('reloadAfterCrashed')
  ? 'true'
  : 'false';

if (
  localStorage.getItem('page-reloaded') === '1' ||
  process.env.RELOAD_AFTER_CRASHED === 'true'
) {
  process.env.RELOAD_AFTER_CRASHED = 'true';
  localStorage.setItem('page-reloaded', '');
}
console.log('RELOAD_AFTER_CRASHED', process.env.RELOAD_AFTER_CRASHED);

process.env.BY_PASS_RESTORE = true;
