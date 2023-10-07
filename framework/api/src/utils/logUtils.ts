import log4js from 'log4js';

import { buildProcessLogPath } from '../config/pathConfig';

log4js.configure({
  appenders: {
    app: {
      type: 'file',
      filename: buildProcessLogPath('app'),
    },
  },
  categories: { default: { appenders: ['app'], level: 'info' } },
});

export const logger = log4js.getLogger('app');

export const kfLogger = {
  info: (...args: Array<unknown>) => {
    if (
      process.env.NODE_ENV === 'development' ||
      process.env.APP_TYPE !== 'cli'
    ) {
      console.log('<KF_INFO>', ...args);
    }
    logger.info('<KF_INFO>', args.join(' '));
  },

  warn: (...args: Array<unknown>) => {
    if (
      process.env.NODE_ENV === 'development' ||
      process.env.APP_TYPE !== 'cli'
    ) {
      console.warn('<KF_WARN>', ...args);
    }
    logger.warn('<KF_WARN>', args.join(' '));
  },

  error: (...args: Array<unknown>) => {
    if (
      process.env.NODE_ENV === 'development' ||
      process.env.APP_TYPE !== 'cli'
    ) {
      console.error('<KF_ERROR>', ...args);
    }
    logger.error('<KF_ERROR>', args.join(' '));
  },
};
