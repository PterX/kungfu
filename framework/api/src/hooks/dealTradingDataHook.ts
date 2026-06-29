import {
  KfCategory,
  UnknownKfCategory,
} from '@kungfu-tech/api/config/tradingConfig';
import {
  KfCategoryEnum,
  KfCategoryTypes,
} from '@kungfu-tech/api/typings/enums';
import { dealTradingData } from '@kungfu-tech/api/utils/busiUtils';
import { kfLogger } from '@kungfu-tech/api/utils/logUtils';
import { generateLocationCombinations } from '@kungfu-tech/api/hooks/hookUtils';

export interface DealTradingDataGetter {
  category: KfCategoryTypes | string;
  commonData: KungfuApi.KfTradeValueCommonData;

  order: {
    getter: (
      watcher: KungfuApi.Watcher,
      orders: KungfuApi.DataTable<KungfuApi.Order>,
      kfLocation: KungfuApi.KfLocation,
    ) => KungfuApi.Order[];
  };

  trade: {
    getter: (
      watcher: KungfuApi.Watcher,
      trades: KungfuApi.DataTable<KungfuApi.Trade>,
      kfLocation: KungfuApi.KfLocation,
    ) => KungfuApi.Trade[];
  };

  position: {
    getter: (
      watcher: KungfuApi.Watcher,
      positions: KungfuApi.DataTable<KungfuApi.Position>,
      kfLocation: KungfuApi.KfLocation,
    ) => KungfuApi.Position[];
  };
}

const DefaultTdDealTrdaingDataHook = {
  category: 'td',
  commonData: KfCategory[KfCategoryEnum.td],
  order: {
    getter: (
      watcher: KungfuApi.Watcher,
      orders: KungfuApi.DataTable<KungfuApi.Order>,
      kfLocation: KungfuApi.KfLocation,
    ) => {
      return dealTradingData<KungfuApi.Order>(
        watcher,
        orders,
        'Order',
        kfLocation,
      ) as KungfuApi.Order[];
    },
  },
  trade: {
    getter: (
      watcher: KungfuApi.Watcher,
      trades: KungfuApi.DataTable<KungfuApi.Trade>,
      kfLocation: KungfuApi.KfLocation,
    ) => {
      return dealTradingData<KungfuApi.Trade>(
        watcher,
        trades,
        'Trade',
        kfLocation,
      ) as KungfuApi.Trade[];
    },
  },
  position: {
    getter: (
      watcher: KungfuApi.Watcher,
      positions: KungfuApi.DataTable<KungfuApi.Position>,
      kfLocation: KungfuApi.KfLocation,
    ) => {
      return dealTradingData<KungfuApi.Position>(
        watcher,
        positions,
        'Position',
        kfLocation,
      ) as KungfuApi.Position[];
    },
  },
};

const DefaultStrategyDealTrdaingDataHook = {
  category: 'strategy',
  commonData: KfCategory[KfCategoryEnum.strategy],
  order: {
    getter: (
      watcher: KungfuApi.Watcher,
      orders: KungfuApi.DataTable<KungfuApi.Order>,
      kfLocation: KungfuApi.KfLocation,
    ) => {
      return dealTradingData<KungfuApi.Order>(
        watcher,
        orders,
        'Order',
        kfLocation,
      ) as KungfuApi.Order[];
    },
  },
  trade: {
    getter: (
      watcher: KungfuApi.Watcher,
      trades: KungfuApi.DataTable<KungfuApi.Trade>,
      kfLocation: KungfuApi.KfLocation,
    ) => {
      return dealTradingData<KungfuApi.Trade>(
        watcher,
        trades,
        'Trade',
        kfLocation,
      ) as KungfuApi.Trade[];
    },
  },
  position: {
    getter: (
      watcher: KungfuApi.Watcher,
      positions: KungfuApi.DataTable<KungfuApi.Position>,
      kfLocation: KungfuApi.KfLocation,
    ) => {
      return dealTradingData<KungfuApi.Position>(
        watcher,
        positions,
        'Position',
        kfLocation,
      ) as KungfuApi.Position[];
    },
  },
};

const DefaultOperatorDealTrdaingDataHook = {
  category: 'operator',
  commonData: KfCategory[KfCategoryEnum.operator],
  order: {
    getter: (
      watcher: KungfuApi.Watcher,
      orders: KungfuApi.DataTable<KungfuApi.Order>,
      kfLocation: KungfuApi.KfLocation,
    ) => {
      return dealTradingData<KungfuApi.Order>(
        watcher,
        orders,
        'Order',
        kfLocation,
      ) as KungfuApi.Order[];
    },
  },
  trade: {
    getter: (
      watcher: KungfuApi.Watcher,
      trades: KungfuApi.DataTable<KungfuApi.Trade>,
      kfLocation: KungfuApi.KfLocation,
    ) => {
      return dealTradingData<KungfuApi.Trade>(
        watcher,
        trades,
        'Trade',
        kfLocation,
      ) as KungfuApi.Trade[];
    },
  },
  position: {
    getter: (
      watcher: KungfuApi.Watcher,
      positions: KungfuApi.DataTable<KungfuApi.Position>,
      kfLocation: KungfuApi.KfLocation,
    ) => {
      return dealTradingData<KungfuApi.Position>(
        watcher,
        positions,
        'Position',
        kfLocation,
      ) as KungfuApi.Position[];
    },
  },
};

const DefaultUnkownDealTrdaingDataHook = {
  category: 'Unknown',
  commonData: UnknownKfCategory,

  order: {
    getter: (
      // eslint-disable-next-line
      ..._args: [
        watcher: KungfuApi.Watcher,
        orders: KungfuApi.DataTable<KungfuApi.Order>,
        kfLocation: KungfuApi.KfLocation,
      ]
    ) => [] as KungfuApi.Order[],
  },

  trade: {
    getter: (
      // eslint-disable-next-line
      ..._args: [
        watcher: KungfuApi.Watcher,
        trades: KungfuApi.DataTable<KungfuApi.Trade>,
        kfLocation: KungfuApi.KfLocation,
      ]
    ) => [] as KungfuApi.Trade[],
  },

  position: {
    getter: (
      // eslint-disable-next-line
      ..._args: [
        watcher: KungfuApi.Watcher,
        positions: KungfuApi.DataTable<KungfuApi.Position>,
        kfLocation: KungfuApi.KfLocation,
      ]
    ) => [] as KungfuApi.Position[],
  },
};

export class DealTradingDataHooks {
  hooks: Record<string, DealTradingDataGetter>;
  constructor() {
    this.hooks = new Proxy(
      {
        'td_*_*_*': DefaultTdDealTrdaingDataHook,
        'strategy_*_*_*': DefaultStrategyDealTrdaingDataHook,
        'operator_*_*_*': DefaultOperatorDealTrdaingDataHook,
      },
      {
        get(target: Record<string, DealTradingDataGetter>, prop: string) {
          const locationPairs = prop.split('_');
          if (locationPairs.length != 4) {
            kfLogger.warn(`Invalid hook key: ${prop}`);
            return [];
          }
          const [category, group, name, mode] = prop.split('_');
          const originalKeys: [string, string, string, string] = [
            category,
            group,
            name,
            mode,
          ];

          const findMatchingKey = () => {
            for (const key of generateLocationCombinations(originalKeys)) {
              if (target[key]) {
                return target[key];
              }
            }
            // eslint-disable-next-line
            return (_key: string) => DefaultUnkownDealTrdaingDataHook;
          };

          return findMatchingKey();
        },

        set(
          target: Record<string, DealTradingDataGetter>,
          prop: string,
          value: DealTradingDataGetter,
        ) {
          if (Reflect.has(target, prop)) {
            kfLogger.warn(`DealTradingData hook ${prop} already exists`);
            return true;
          }

          kfLogger.info(`DealTradingData hook ${prop} register success`);
          Reflect.set(target, prop, value);
          return true;
        },
      },
    );
  }

  register(
    kfLocation: KungfuApi.DerivedKfLocation,
    getter: DealTradingDataGetter,
  ) {
    const { category, group, name, mode } = kfLocation;
    const key = `${category}_${group}_${name}_${mode}`;
    Reflect.set(this.hooks, key, getter);
  }

  trigger(
    watcher: KungfuApi.Watcher | null,
    kfLocation: KungfuApi.DerivedKfLocation,
    dataSource: KungfuApi.TradingDataTable,
    tradingDataType: 'order' | 'trade' | 'position',
  ) {
    if (!watcher) {
      kfLogger.warn('Watcher is NULL');
      return [];
    }

    const { category, group, name, mode } = kfLocation;
    const key = `${category}_${group}_${name}_${mode}`;
    const getter = Reflect.get(this.hooks, key)[tradingDataType].getter;
    return getter(
      watcher,
      dataSource as KungfuApi.MergedTradingDataTable,
      kfLocation,
    );
  }

  getCategoryMap(): Record<string, KungfuApi.KfTradeValueCommonData> {
    return Object.keys(this.hooks).reduce((pre, key) => {
      const [category, group, name, mode] = key.split('_');
      if (group === name && group === '*' && mode === '*') {
        pre[category] = Reflect.get(this.hooks, key).commonData;
      }
      return pre;
    }, {});
  }
}
