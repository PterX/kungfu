import '@kungfu-tech/api/setGlobalEnv';
import dayjs from 'dayjs';

process.env.APP_ID = dayjs().format('MMDDHHmmss');
process.env.BY_PASS_RESTORE = true;
process.env.BY_PASS_ACCOUNTING = true;
process.env.BY_PASS_TRADINGDATA = true;
process.env.BY_PASS_REFRESHBOOK = true;
process.env.MILLISECONDS_SLEEP_AFTER_STEP = 100;
