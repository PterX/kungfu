import { Subject } from 'rxjs';
import { Locale } from 'ant-design-vue/es/locale-provider';

declare module '@vue/runtime-core' {
  interface ComponentCustomProperties {
    $antLocalesMap: Record<string, Locale>;
    $globalBus: Subject<KfBusEvent>;
    $tradingDataSubject: Subject<{
      watcher: KungfuApi.Watcher;
      tradingDataKeeper: KungfuApi.TradingDataKeeper;
    }>;
    $availKfBoards: string[];
    _: any; //vue3 shit
  }
}

declare module 'worker-loader!*' {
  class WebpackWorker extends Worker {
    constructor();
  }

  export default WebpackWorker;
}

export {};
