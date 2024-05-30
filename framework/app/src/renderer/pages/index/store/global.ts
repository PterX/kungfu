import { defineStore } from 'pinia';
import { toRaw } from 'vue';
import {
  getKfUIExtensionConfig,
  getKfExtensionConfig,
} from '@kungfu-trader/kungfu-js-api/utils/extUtils';
import {
  getIdByKfLocation,
  getProcessIdByKfLocation,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  getAllKfConfigOriginData,
  getAllRiskSettingList,
  getSubscribedInstruments,
  getTdGroups,
  getAllBaskets,
} from '@kungfu-trader/kungfu-js-api/actions';
import {
  Pm2ProcessStatusDetailData,
  Pm2ProcessStatusData,
} from '@kungfu-trader/kungfu-js-api/utils/processUtils';
import {
  BasketTypeEnum,
  BrokerStateStatusTypes,
  KfCategoryTypes,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import globalBus from '@kungfu-trader/kungfu-js-api/utils/globalBus';
import {
  SideEnum,
  OffsetEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import { getKfGlobalSettingsValue } from '@kungfu-trader/kungfu-js-api/config/globalSettings';

interface GlobalState {
  currentBoardsStoreId: string;
  extConfigs: KungfuApi.KfExtConfigs;
  uiExtConfigs: KungfuApi.KfUIExtConfigs;
  tdList: KungfuApi.KfConfig[];
  tdGroupList: KungfuApi.KfExtraLocation[];
  mdList: KungfuApi.KfConfig[];
  strategyList: KungfuApi.KfConfig[];
  operatorList: KungfuApi.KfConfig[];
  basketList: KungfuApi.Basket[];

  coreBindRoutePaths: Set<string>; // 依赖于底层的页面
  preStartSystemLoadingData: Record<
    'archive' | 'watcher' | 'extraResourcesLoading' | 'cpusSafeNumChecking',
    'loading' | 'done'
  >;
  preQuitSystemLoadingData: Record<
    'record' | 'quit',
    'loading' | 'done' | undefined
  >;

  processStatusData: Pm2ProcessStatusData;
  processStatusWithDetail: Pm2ProcessStatusDetailData;

  appStates: Record<string, BrokerStateStatusTypes>;
  strategyStates: Record<string, KungfuApi.StrategyStateData>;

  assets: Record<string, KungfuApi.Asset>;
  instruments: KungfuApi.InstrumentResolved[];
  instrumentsMap: Record<string, KungfuApi.InstrumentResolved>;
  subscribedInstrumentsByLocal: KungfuApi.InstrumentResolved[];
  curSubscribedInstruments: Record<string, boolean>;

  riskSettings: KungfuApi.RiskSetting[];

  globalSetting: Record<string, Record<string, KungfuApi.KfConfigValue>>;

  testCase: {
    replayEnabled: Partial<Record<KfCategoryTypes | 'ledger', boolean>>;
  };

  currentGlobalKfLocation:
    | KungfuApi.KfLocation
    | KungfuApi.KfConfig
    | KungfuApi.KfExtraLocation
    | null;

  orderBookCurrentInstrument: KungfuApi.InstrumentResolved | undefined;

  globalFormState: {
    account_id?: string;
    instrument?: string;
    volume?: number;
    side?: SideEnum;
    offset?: OffsetEnum;
  };

  tdFilter: ((td: KungfuApi.KfConfig[]) => KungfuApi.KfConfig[]) | null;
}

export const useGlobalStore = defineStore('global', {
  state: (): GlobalState => {
    return {
      currentBoardsStoreId: 'main',
      extConfigs: toRaw<KungfuApi.KfExtConfigs>({
        td: {},
        md: {},
        strategy: {},
        operator: {},
        system: {},
      }),
      uiExtConfigs: toRaw<KungfuApi.KfUIExtConfigs>({}),
      tdFilter: null,
      tdList: [],
      tdGroupList: [],
      mdList: [],
      strategyList: [],
      operatorList: [],
      basketList: [],

      coreBindRoutePaths: new Set(),
      preStartSystemLoadingData: {
        archive: 'loading',
        watcher: 'loading',
        extraResourcesLoading: 'loading',
        cpusSafeNumChecking: 'loading',
      },
      preQuitSystemLoadingData: {
        record: undefined,
        quit: undefined,
      },

      processStatusData: {},
      processStatusWithDetail: {},

      appStates: {},
      strategyStates: {},
      assets: {},
      instruments: [],
      instrumentsMap: {},
      subscribedInstrumentsByLocal: [],
      curSubscribedInstruments: {},

      riskSettings: [],

      globalSetting: {},

      testCase: {
        replayEnabled: {
          td: false,
          ledger: true,
          strategy: true,
          operator: true,
        },
      },

      currentGlobalKfLocation: null,
      orderBookCurrentInstrument: undefined,

      globalFormState: {},
    };
  },

  actions: {
    setTdGroups() {
      return getTdGroups().then((tdGroups) => {
        this.tdGroupList = tdGroups;
        globalBus.next({
          tag: 'update:tdGroup',
          tdGroups: this.tdGroupList,
        });
      });
    },

    setSubscribedInstrumentsByLocal() {
      getSubscribedInstruments().then((instruments) => {
        this.subscribedInstrumentsByLocal = toRaw(instruments);
      });
    },

    setCurSubscribedInstruments(newInstrumentsMap: Record<string, boolean>) {
      Object.assign(this.curSubscribedInstruments, newInstrumentsMap);
    },

    setInstruments(instruments: KungfuApi.InstrumentResolved[]) {
      this.instruments = toRaw(instruments);
    },

    setInstrumentsMap(
      instrumentsMap: Record<string, KungfuApi.InstrumentResolved>,
    ) {
      this.instrumentsMap = toRaw(instrumentsMap);
    },

    setTestCase(key: KfCategoryTypes | 'ledger', value: boolean) {
      this.testCase.replayEnabled[key] = value;
    },

    setCurrentGlobalKfLocation(
      kfLocation:
        | KungfuApi.KfLocation
        | KungfuApi.KfConfig
        | KungfuApi.KfExtraLocation
        | null,
    ) {
      this.currentGlobalKfLocation = kfLocation;
    },

    setOrderBookCurrentInstrument(
      instrument: KungfuApi.InstrumentResolved | undefined,
    ) {
      this.orderBookCurrentInstrument = instrument;
    },

    setGlobalFormState(formState: GlobalState['globalFormState']) {
      Object.assign(this.globalFormState, formState);
    },

    setAppStates(appStates: Record<string, BrokerStateStatusTypes>) {
      this.appStates = appStates;
    },

    setStrategyStates(
      strategyStates: Record<string, KungfuApi.StrategyStateData>,
    ) {
      this.strategyStates = strategyStates;
    },

    setAssets(assets: Record<string, KungfuApi.Asset>) {
      this.assets = assets;
    },

    setProcessStatus(processStatus: Pm2ProcessStatusData) {
      this.processStatusData = toRaw(processStatus);
    },

    setProcessStatusWithDetail(
      processStatusWithDetail: Pm2ProcessStatusDetailData,
    ) {
      this.processStatusWithDetail = toRaw(processStatusWithDetail);
    },

    setKfConfigList() {
      return getAllKfConfigOriginData(window.watcher).then((res) => {
        const { md, strategy, operator } = res;
        let { td } = res;
        if (this.tdFilter) {
          td = this.tdFilter(td);
        }
        this.mdList = md;
        this.tdList = td;
        this.strategyList = strategy;
        this.operatorList = operator;

        globalBus.next({
          tag: 'update:td',
          tds: td,
        });

        globalBus.next({
          tag: 'update:md',
          mds: md,
        });

        globalBus.next({
          tag: 'update:strategy',
          strategys: strategy,
        });

        globalBus.next({
          tag: 'update:operator',
          operators: operator,
        });

        this.setDefaultCurrentGlobalKfLocation();
      });
    },

    setKfGlobalSetting() {
      this.globalSetting = getKfGlobalSettingsValue();
    },

    setRiskSettingList() {
      return getAllRiskSettingList(window.watcher).then((res) => {
        this.riskSettings = res;
      });
    },

    setBasketList() {
      return getAllBaskets(window.watcher).then((basketList) => {
        this.basketList = basketList.filter(
          (item) => item.basket_type === BasketTypeEnum.Custom,
        );
      });
    },

    checkCurrentGlobalKfLocationExisted() {
      if (!this.currentGlobalKfLocation) {
        return false;
      }

      const categoryToKfConfigsMap: Record<
        KfCategoryTypes,
        KungfuApi.KfConfig[]
      > = {
        td: this.tdList,
        md: this.mdList,
        strategy: this.strategyList,
        system: [],
        operator: [],
      };

      const targetKfConfigs: KungfuApi.KfConfig[] =
        categoryToKfConfigsMap[
          this.currentGlobalKfLocation.category as KfCategoryTypes
        ];
      if (!targetKfConfigs || !targetKfConfigs.length) {
        return false;
      }

      const afterFilter: KungfuApi.KfConfig[] = targetKfConfigs.filter(
        (item) => {
          if (
            this.currentGlobalKfLocation &&
            getIdByKfLocation(item) ===
              getIdByKfLocation(this.currentGlobalKfLocation)
          ) {
            return true;
          }

          return false;
        },
      );

      return afterFilter.length > 0;
    },

    setDefaultCurrentGlobalKfLocation() {
      if (
        this.currentGlobalKfLocation?.category === 'strategy' &&
        this.currentGlobalKfLocation.group !== 'default' &&
        this.processStatusWithDetail[
          getProcessIdByKfLocation(this.currentGlobalKfLocation)
        ]
      ) {
        return;
      }

      if (
        !this.currentGlobalKfLocation ||
        !this.checkCurrentGlobalKfLocationExisted()
      ) {
        if (this.tdList.length) {
          this.setCurrentGlobalKfLocation(this.tdList[0]);
        } else if (this.strategyList.length) {
          this.setCurrentGlobalKfLocation(this.strategyList[0]);
        } else {
          this.setCurrentGlobalKfLocation(null);
        }
      }
    },

    setKfExtConfigs() {
      return getKfExtensionConfig().then(
        (kfExtConfigs: KungfuApi.KfExtConfigs) => {
          globalBus.next({
            tag: 'update:extConfigs',
            extConfigs: kfExtConfigs,
          });
          this.extConfigs = toRaw(kfExtConfigs);
        },
      );
    },

    setKfUIExtConfigs() {
      return getKfUIExtensionConfig().then(
        (kfUiExtConfig: KungfuApi.KfUIExtConfigs) => {
          this.uiExtConfigs = toRaw(kfUiExtConfig);
          return kfUiExtConfig;
        },
      );
    },

    setTdFilter(tdFilter: (tds: KungfuApi.KfConfig[]) => KungfuApi.KfConfig[]) {
      this.tdFilter = tdFilter;
    },
  },

  getters: {
    instrumentKeyAccountsMap(): Record<string, string[]> {
      const instrumentKeyAccountsMap: Record<string, string[]> = {};
      this.riskSettings.forEach((riskListItem: KungfuApi.RiskSetting) => {
        if (riskListItem.white_list && riskListItem.white_list.length) {
          riskListItem.white_list.forEach((instrument) => {
            if (
              !instrumentKeyAccountsMap[instrument] ||
              !instrumentKeyAccountsMap[instrument].length
            ) {
              instrumentKeyAccountsMap[instrument] = [];
            }
            instrumentKeyAccountsMap[instrument].push(riskListItem.account_id);
          });
        }
      });
      return instrumentKeyAccountsMap;
    },
  },
});
