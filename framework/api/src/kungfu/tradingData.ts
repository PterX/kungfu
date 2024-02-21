import { Subject } from 'rxjs';
import { useWatcher, startWatcher } from './watcher';
const { startWatcherSyncTask } = useWatcher();
export const tradingDataSubject = new Subject<{
  watcher: KungfuApi.Watcher;
  tradingDataObject: KungfuApi.TradingDataObject;
  update: boolean;
}>();

export const triggerStartStep = (stepInterval = 2000) => {
  startWatcher();
  startWatcherSyncTask(stepInterval, (watcher, tradingDataObject, update) => {
    tradingDataSubject.next({ watcher, tradingDataObject, update });
  });
};
