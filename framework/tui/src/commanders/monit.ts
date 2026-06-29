import { selectTargetKfConfig } from '../assets/methods/utils';
import monitor from '../components/monitor';
import tradingDataMonitor from '../components/tradingDataMonitor';
import { globalState } from '../assets/actions/globalState';
import { dealAppStates } from '@kungfu-tech/api/utils/busiUtils';
import { setTimerPromiseTask } from '@kungfu-tech/api/utils/commonUtils';

export const monitPrompt = async (list: boolean) => {
  const { watcher } = await import(
    '@kungfu-tech/api/kungfu/watcher'
  );
  const { triggerStartStep } = await import(
    '@kungfu-tech/api/kungfu/tradingData'
  );

  triggerStartStep();

  setTimerPromiseTask((): Promise<void> => {
    return new Promise((resolve) => {
      globalState.APP_STATES_SUBJECT.next(
        dealAppStates(watcher, watcher?.appStates || {}),
      );
      resolve();
    });
  }, 3000);

  const timer = setTimeout(() => {
    globalState.APP_STATES_SUBJECT.next({});
    clearTimeout(timer);
  }, 1000);

  process.on('SIGINT', () => {
    watcher?.quit();
    process.exit();
  });

  process.on('exit', () => {
    watcher?.quit();
  });

  if (list) {
    const kfConfig = await selectTargetKfConfig(true);

    if (!kfConfig) {
      throw new Error('target is illegal kfLocation');
    }

    // it is so important, because inquirer event will conflict with blessed
    process.stdin.removeAllListeners('data');
    return tradingDataMonitor(kfConfig);
  }

  process.stdin.removeAllListeners('data');
  return monitor();
};
