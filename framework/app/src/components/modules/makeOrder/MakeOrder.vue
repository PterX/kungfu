<script setup lang="ts">
import {
  computed,
  getCurrentInstance,
  nextTick,
  onMounted,
  ref,
  watch,
} from 'vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';

import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfConfigSettingsForm from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfConfigSettingsForm.vue';
import {
  useTriggerMakeOrder,
  useDashboardBodySize,
  confirmModal,
  messagePrompt,
  useKeyboardControlContainerStyle,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { useActiveInstruments } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { getConfigSettings, LABEL_COL, WRAPPER_COL } from './config';
import { dealOrderPlaceVNode, dealStockOffset } from './utils';
import { hashInstrumentUKey } from '@kungfu-trader/kungfu-js-api/kungfu';
import {
  makeOrderByOrderInput,
  getPosClosableVolume,
  makeOrderByOrderTriggerInput,
  getPrecisionByInstrumentType,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import {
  InstrumentTypeEnum,
  OffsetEnum,
  OrderInputKeyEnum,
  SideEnum,
  PriceTypeEnum,
  OrderTriggerConfigTypeEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import {
  Side,
  MarginSideStatus,
} from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import {
  useCurrentGlobalKfLocation,
  useExtConfigsRelated,
  useInstruments,
  useProcessStatusDetailData,
  useTradeLimit,
  useMarginSupport,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import {
  initFormStateByConfig,
  enableCustomRadioType,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import {
  getIdByKfLocation,
  getProcessIdByKfLocation,
  dealKfDecimalPrecision,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  isShotable,
  dealOrderInputItem,
  transformSearchInstrumentResultToInstrument,
  dealVolumeByInstrumentType,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import OrderConfirmModal from './OrderConfirmModal.vue';
import OrderTriggerConfirmModal from './OrderTriggerConfirmModal.vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { resolveTriggerOffset } from '../pos/utils';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';
import { storeToRefs } from 'pinia';
import {
  useMakeOrderInfo,
  useMakeOrderSubscribe,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { readRootPackageJsonSync } from '@kungfu-trader/kungfu-js-api/utils/fileUtils';

const { t } = VueI18n.global;
const { error, success } = messagePrompt();
const app = getCurrentInstance();
const {
  currentGlobalKfLocation,
  currentCategoryData,
  getCurrentGlobalKfLocationId,
} = useCurrentGlobalKfLocation(window.watcher);

const { getPriceTickAndPrecision, getQuantityUnitAndPrecision } =
  useActiveInstruments();
const { globalSetting, instrumentsMap } = storeToRefs(useGlobalStore());
const { handleBodySizeChange } = useDashboardBodySize();
const { mdExtTypeMap, extConfigs } = useExtConfigsRelated();

const formRef = ref();
const boardRef = ref();
const makeOrderRef = ref();
const apartOrderRef = ref();
const orderTriggerRef = ref();

useKeyboardControlContainerStyle(
  'MakeOrder',
  '.ant-form-item-control-input:focus-within { background: rgba(67, 67, 67, 0.3); }',

  boardRef,
  formRef,
);

const currentAccountId = ref<string>('');

const formState = ref(
  initFormStateByConfig(
    getConfigSettings({
      location: currentGlobalKfLocation.value,
      instrumentType: InstrumentTypeEnum.future,
      isMarginMakeOrder: false,
      isSpecifyContract: false,
    }),
    {},
  ),
);

const { isMarginMakeOrder, isSpecifyContract } = useMarginSupport(
  currentGlobalKfLocation,
  formState,
);

const sideList = ref<string[]>([SideEnum.Buy + '', SideEnum.Sell + '']);
const offsetList = ref<string[]>(Object.keys(enableCustomRadioType['offset']));

const autoFillInstrument = ref<boolean>(false);

const { subscribeAllInstrumentByAppStates } = useInstruments();
const { appStates, processStatusData } = useProcessStatusDetailData();

const { triggerOrderBook } = useTriggerMakeOrder();
const {
  showAmountOrPosition,
  instrumentResolved,
  currentPositionWithLongDirection,
  currentPositionWithShortDirection,
  currentPosition,
  currentResidueMoney,
  currentResiduePosVolume,
  currentPrice,
  currentTradeAmount,
  currentAvailMoney,
  currentAvailPosVolume,
  isAccountOrInstrumentConfirmed,
} = useMakeOrderInfo(formState, isMarginMakeOrder);
useMakeOrderSubscribe(formState);

const availablePosOrAmount = computed(() => {
  return showAmountOrPosition.value === 'amount'
    ? currentAvailMoney.value
    : currentAvailPosVolume.value;
});

const leftPosOrAmount = computed(() => {
  return showAmountOrPosition.value === 'amount'
    ? currentResidueMoney.value
    : currentResiduePosVolume.value;
});

const { getValidatorByOrderInputKey } = useTradeLimit();

const makeOrderInstrumentType = ref<InstrumentTypeEnum>(
  InstrumentTypeEnum.unknown,
);

const tdList = computed<KungfuApi.KfLocation[] | null | undefined>(() => {
  return currentGlobalKfLocation.value &&
    'children' in currentGlobalKfLocation.value
    ? currentGlobalKfLocation.value.children
    : null;
});

const configSettings = computed(() => {
  if (!currentGlobalKfLocation.value) {
    return getConfigSettings({});
  }

  let priceStep = 1,
    pricePrecision = 0,
    volumePrecision = 0,
    volumeStep = 1;
  if (instrumentResolved.value) {
    const { instrumentId, exchangeId } = instrumentResolved.value;
    const { quantity_unit, volume_precision } = getQuantityUnitAndPrecision(
      instrumentId,
      exchangeId,
    );
    const { price_tick, price_precision } = getPriceTickAndPrecision(
      instrumentId,
      exchangeId,
    );
    priceStep = price_tick;
    pricePrecision = price_precision;
    volumeStep = quantity_unit;
    volumePrecision = volume_precision;
  }

  const { side } = formState.value;
  return getConfigSettings({
    location: currentGlobalKfLocation.value,
    instrumentType: makeOrderInstrumentType.value,
    isMarginMakeOrder: isMarginMakeOrder.value,
    isSpecifyContract: isSpecifyContract.value,
    side,
    priceType: +formState.value.price_type,
    pricePrecision: pricePrecision || null,
    priceStep,
    sideList: sideList.value,
    offsetList: offsetList.value,
    volumeStep,
    volumePrecision,
  });
});

const rules = computed(() => {
  const { instrument } = formState.value;
  return {
    volume: {
      validator: getValidatorByOrderInputKey(
        OrderInputKeyEnum.VOLUME,
        instrument,
      ),
      trigger: 'change',
    },
    limit_price: {
      validator: getValidatorByOrderInputKey(
        OrderInputKeyEnum.PRICE,
        instrument,
      ),
      trigger: 'change',
    },
  };
});
const isShowConfirmModal = ref<boolean>(false);
const curOrderVolume = ref<number>(0);
const curOrderType = ref<InstrumentTypeEnum>(InstrumentTypeEnum.unknown);
const currentPercent = ref<number>(0);
const percentList = [10, 20, 50, 80, 100];

const makeOrderData = computed(() => {
  if (!instrumentResolved.value) {
    return null;
  }

  const { exchangeId, instrumentId, instrumentType } = instrumentResolved.value;

  const {
    limit_price,
    volume,
    price_type,
    side,
    offset,
    hedge_flag,
    is_swap,
    contract_id,
  } = formState.value;

  const makeOrderInput: KungfuApi.MakeOrderInput = {
    instrument_id: instrumentId,
    instrument_type: +instrumentType,
    exchange_id: exchangeId,
    limit_price: +limit_price,
    volume: +volume,
    price_type: +price_type,
    side: +side,
    offset: getResolvedOffset(offset, side, instrumentType),
    hedge_flag: +(hedge_flag || 0),
    is_swap: !!is_swap,
    parent_id: 0n,
    contract_id: contract_id || '',
  };
  return makeOrderInput;
});

const getResolvedOffset = (
  offset: OffsetEnum,
  side: SideEnum,
  instrumentType: InstrumentTypeEnum,
) => {
  if (isShotable(instrumentType) || isMarginMakeOrder.value) {
    if (offset !== undefined) {
      return offset;
    }
  }
  if (isMarginMakeOrder.value) {
    if (
      [
        SideEnum.GuaranteeStockBuy,
        SideEnum.MarginTrade,
        SideEnum.RepayStock,
      ].includes(side)
    ) {
      return 0;
    } else {
      return 1;
    }
  } else {
    if (side === 0) {
      return 0;
    } else {
      return 1;
    }
  }
};

watch(
  () => currentGlobalKfLocation.value,
  (newVal) => {
    if (newVal?.category === 'td') {
      formState.value.account_id = getIdByKfLocation(newVal);
    } else {
      formState.value.account_id = '';
    }
  },
);

watch(
  [() => formState.value.account_id, () => formState.value.instrument],
  ([newAccountId, newInstrument]) => {
    if (!newInstrument || !currentGlobalKfLocation.value) return;
    const instrumentResolved =
      transformSearchInstrumentResultToInstrument(newInstrument);
    if (instrumentResolved) {
      const { instrumentType, exchangeId } = instrumentResolved;

      const tdName = newAccountId ? newAccountId.split('_')[0] : '';

      const extConfig = extConfigs.value.td[tdName];
      if (instrumentType === InstrumentTypeEnum.stockoption) {
        sideList.value = [...Object.keys(Side).slice(0, 2), SideEnum.Exec + ''];
      } else if (
        instrumentType === InstrumentTypeEnum.fund &&
        extConfig &&
        extConfig.supportEtf
      ) {
        sideList.value = [
          ...Object.keys(Side).slice(0, 2),
          SideEnum.Purchase + '',
          SideEnum.Redemption + '',
        ];
      } else {
        sideList.value = Object.keys(Side).slice(0, 2);
      }

      if (instrumentType === InstrumentTypeEnum.future) {
        if (exchangeId !== 'SHFE' && exchangeId !== 'INE') {
          offsetList.value = offsetList.value.filter(
            (item) =>
              item !== OffsetEnum.CloseToday + '' ||
              item !== OffsetEnum.CloseYest + '',
          );
        }
      }

      if (
        !isMarginMakeOrder.value &&
        'side' in formState.value &&
        !sideList.value.includes(formState.value.side + '')
      ) {
        formState.value.side = +sideList.value[0];
      }

      if (formState.value.contract_id && !autoFillInstrument.value) {
        formState.value.contract_id = '';
      } else {
        autoFillInstrument.value = false;
      }

      subscribeAllInstrumentByAppStates(
        processStatusData.value,
        appStates.value,
        mdExtTypeMap.value,
        [instrumentResolved],
      );
      triggerOrderBook(instrumentResolved);

      makeOrderInstrumentType.value = instrumentResolved.instrumentType;
    }
  },
);

watch(
  () => isMarginMakeOrder.value,
  (newVal) => {
    if (newVal) {
      if (!MarginSideStatus.includes(formState.value.side)) {
        formState.value.side = SideEnum.GuaranteeStockBuy;
      }
    } else {
      if (MarginSideStatus.includes(formState.value.side)) {
        formState.value.side = SideEnum.Buy;
      }
    }
  },
  {
    immediate: true,
  },
);

watch(
  () => formState.value.side,
  (newSide) => {
    if (isMarginMakeOrder.value) {
      [
        SideEnum.GuaranteeStockBuy,
        SideEnum.MarginTrade,
        SideEnum.ShortSell,
      ].includes(formState.value.side)
        ? (formState.value.offset = OffsetEnum.Open)
        : (formState.value.offset = OffsetEnum.Close);
      if (
        formState.value.side !== SideEnum.RepayStock ||
        formState.value.side !== SideEnum.RepayMargin
      ) {
        formState.value.contract_id = '';
      }

      if (
        !isSpecifyContract.value &&
        formState.value.side === SideEnum.RepayMargin
      ) {
        formState.value.contract_id = '';
      }
    } else {
      if (instrumentResolved.value) {
        const { instrumentType } = instrumentResolved.value;

        if (isShotable(instrumentType)) {
          if (newSide === SideEnum.Sell) {
            if (currentPositionWithLongDirection.value) {
              formState.value.offset = currentPositionWithLongDirection.value
                ? resolveTriggerOffset(currentPositionWithLongDirection.value)
                : OffsetEnum.Open;
            }
          } else if (newSide === SideEnum.Buy) {
            formState.value.offset = currentPositionWithShortDirection.value
              ? resolveTriggerOffset(currentPositionWithShortDirection.value)
              : OffsetEnum.Open;
          }
        } else {
          formState.value.offset =
            newSide === SideEnum.Buy ? OffsetEnum.Open : OffsetEnum.Close;
        }
      }
    }
  },
);

watch(
  () => formState.value.contract_id,
  (newVal) => {
    try {
      if (newVal) {
        const contractList = window.watcher.ledger.Contract.filter(
          'contract_id',
          newVal,
        ).list();
        if (contractList.length === 0) {
          return;
        }

        const { instrument_id, exchange_id } = contractList[0];
        const ukey = hashInstrumentUKey(instrument_id, exchange_id);
        const instrumentResolved = instrumentsMap.value[ukey];
        if (!instrumentResolved) {
          return;
        }

        const instrumentStr = `${instrumentResolved.exchangeId}_${instrumentResolved.instrumentId}_${instrumentResolved.instrumentType}_${ukey}_${instrumentResolved.instrumentName}`;
        if (formState.value.instrument !== instrumentStr) {
          formState.value.instrument = instrumentStr;
          autoFillInstrument.value = true;
        }
      }
    } catch (error) {
      console.error(error);
    }
  },
);

watch(
  () => formState.value,
  (newVal) => {
    let { account_id, instrument, volume, side, offset } = newVal;
    if (![SideEnum.Buy, SideEnum.Sell].includes(side)) {
      side = undefined;
    }
    useGlobalStore().setGlobalFormState({
      account_id,
      instrument,
      volume,
      side,
      offset,
    });
  },
);

watch(
  () => formState.value.price_type,
  (newVal: PriceTypeEnum) => {
    if (newVal === PriceTypeEnum.AtAuction) {
      const limitPriceIndex = configSettings.value.findIndex((configItem) => {
        return configItem.key === 'limit_price';
      });
      configSettings.value.splice(limitPriceIndex, 1);
    }
  },
);

watch(
  () => formState.value.account_id,
  (newVal) => {
    currentAccountId.value = newVal;
  },
);

onMounted(() => {
  if (currentGlobalKfLocation.value?.category === 'td') {
    formState.value.account_id = getIdByKfLocation(
      currentGlobalKfLocation.value,
    );
    formState.value.offset = OffsetEnum.Open;
  } else {
    formState.value.account_id = '';
  }
});

// 下单操作
function placeOrder(
  orderInput: KungfuApi.MakeOrderInput,
  globalKfLocation: KungfuApi.KfLocation,
  tdProcessId: string,
): Promise<bigint> {
  return makeOrderByOrderInput(
    window.watcher,
    orderInput,
    globalKfLocation,
    tdProcessId.toAccountId(),
  );
}

function initOrderInputData(): Promise<KungfuApi.MakeOrderInput> {
  if (!instrumentResolved.value) {
    return Promise.reject(new Error(t('instrument_error')));
  }

  const { exchangeId, instrumentId, instrumentType } = instrumentResolved.value;
  const {
    contract_id,
    limit_price,
    volume,
    price_type,
    side,
    offset,
    hedge_flag,
    is_swap,
  } = formState.value;

  const makeOrderInput: KungfuApi.MakeOrderInput = {
    instrument_id: instrumentId,
    instrument_type: +instrumentType,
    exchange_id: exchangeId,
    limit_price: +limit_price,
    volume: +volume,
    price_type: +price_type,
    side: +side,
    offset: getResolvedOffset(offset, side, instrumentType),
    hedge_flag: +(hedge_flag || 0),
    is_swap: !!is_swap,
    parent_id: 0n,
    contract_id: contract_id || '',
  };

  return Promise.resolve(makeOrderInput);
}

function handleResetMakeOrderForm(): void {
  const initFormState = initFormStateByConfig(configSettings.value, {});

  Object.keys(initFormState).forEach((key) => {
    formState.value[key] = initFormState[key];
  });

  nextTick().then(() => {
    formRef.value.clearValidate();
  });
}

// 拆单
async function handleApartOrder(): Promise<void> {
  try {
    await formRef.value.validate();
    const makeOrderInput: KungfuApi.MakeOrderInput = await initOrderInputData();
    const flag = await showCloseModal(makeOrderInput);
    if (!flag) return;
    const isContinue = await confirmContinueOrderModal(
      dealFatFingerMessage(makeOrderInput),
    );
    if (isContinue !== null && !isContinue) {
      apartOrderRef.value?.focus();
      return;
    }

    isShowConfirmModal.value = true;
    curOrderVolume.value = makeOrderInput.volume;
    curOrderType.value = makeOrderInput.instrument_type;
  } catch (e) {
    if ((<Error>e).message) {
      error((<Error>e).message);
    }
  }
}

// 拆单弹窗确认回调
async function handleApartedConfirm(volumeList: number[]): Promise<void> {
  try {
    if (!makeOrderData.value || !currentGlobalKfLocation.value) {
      apartOrderRef.value?.focus();
      return;
    }

    const tdProcessId = await confirmOrderPlace(
      makeOrderData.value,
      volumeList.length,
    );
    if (!tdProcessId) {
      apartOrderRef.value?.focus();
      return;
    }

    const apartOrderInput: KungfuApi.MakeOrderInput = makeOrderData.value;

    Promise.all(
      volumeList.map((volume) => {
        apartOrderInput.volume = volume;
        return placeOrder(
          apartOrderInput as KungfuApi.MakeOrderInput,
          currentGlobalKfLocation.value as KungfuApi.KfLocation,
          tdProcessId,
        );
      }),
    );
    apartOrderRef.value?.focus();
  } catch (e) {
    if ((<Error>e).message) {
      error((<Error>e).message);
    }
  }
}

function confirmContinueOrderModal(
  warnningMessage: string,
  okText = t('tradingConfig.Continue'),
  cancelText = t('cancel'),
): Promise<boolean | null> {
  if (warnningMessage !== '') {
    return confirmModal(t('warning'), warnningMessage, okText, cancelText);
  } else {
    return Promise.resolve(null);
  }
}

function dealFatFingerMessage(
  makeOrderInput: KungfuApi.MakeOrderInput,
): string {
  if (!instrumentResolved.value) {
    return '';
  }

  const fatFingerRange = +globalSetting.value?.trade?.fatFinger || 0;

  if (fatFingerRange === 0) return '';

  const { exchangeId, instrumentId } = instrumentResolved.value;
  const ukey = hashInstrumentUKey(instrumentId, exchangeId);

  const { limit_price: price, side } = makeOrderInput;
  const lastPrice = window.watcher.ledger.Quote[ukey]?.last_price;

  const fatFingerBuyRate = (100 + fatFingerRange) / 100;
  const fatFingerSellRate = (100 - fatFingerRange) / 100;

  if (SideEnum.Buy == side && price > lastPrice * fatFingerBuyRate) {
    return t('tradingConfig.fat_finger_buy_modal', {
      price: price,
      warningLine: (lastPrice * fatFingerBuyRate).kfToFixed(4),
      fatFinger: fatFingerRange,
    });
  }

  if (SideEnum.Sell == side && price < lastPrice * fatFingerSellRate) {
    return t('tradingConfig.fat_finger_sell_modal', {
      price: price,
      warningLine: (lastPrice * fatFingerSellRate).kfToFixed(4),
      fatFinger: fatFingerRange,
    });
  }

  return '';
}

async function confirmOrderPlace(
  makeOrderInput: KungfuApi.MakeOrderInput,
  orderCount: number = 1,
): Promise<string> {
  if (!currentGlobalKfLocation.value || !window.watcher) {
    return Promise.reject(new Error(t('location_error')));
  }

  const { account_id } = formState.value;
  const tdProcessId =
    currentGlobalKfLocation.value?.category === 'td'
      ? getProcessIdByKfLocation(currentGlobalKfLocation.value)
      : getProcessIdByKfLocation({
          category: 'td',
          group: account_id.split('_')[0],
          name: account_id.split('_')[1],
          mode: 'live',
        });

  if (processStatusData.value[tdProcessId] !== 'online') {
    return Promise.reject(
      new Error(t('tradingConfig.start_process', { process: tdProcessId })),
    );
  }
  if (!globalSetting.value?.trade?.skipConfirmMakeOrder) {
    const flag = await confirmModal(
      t('tradingConfig.place_confirm'),
      dealOrderPlaceVNode(makeOrderInput, orderCount, false),
    );

    if (!flag) return Promise.resolve('');
  }

  return Promise.resolve(tdProcessId);
}

async function confirmApartCloseToOpen(
  makeOrderInput: KungfuApi.MakeOrderInput,
) {
  const { side, offset, volume } = makeOrderInput;

  if (
    isShotable(instrumentResolved.value?.instrumentType) &&
    offset !== OffsetEnum.Open
  ) {
    let direction: string = '',
      oppositeDirection: string = '',
      closableVolume: number | undefined = undefined;

    if (side === SideEnum.Buy) {
      if (currentPositionWithShortDirection.value) {
        closableVolume = getPosClosableVolume(
          currentPositionWithShortDirection.value,
        );

        direction = t('tradingConfig.short');
        oppositeDirection = t('tradingConfig.long');
      }
    } else if (side === SideEnum.Sell) {
      if (currentPositionWithLongDirection.value) {
        closableVolume = getPosClosableVolume(
          currentPositionWithLongDirection.value,
        );

        direction = t('tradingConfig.long');
        oppositeDirection = t('tradingConfig.short');
      }
    }

    if (direction === '' || closableVolume === undefined)
      return [makeOrderInput];

    if (volume > closableVolume) {
      const precision = getPrecisionByInstrumentType(
        makeOrderInstrumentType.value,
      );
      const openVolume = dealKfDecimalPrecision(
        volume - closableVolume,
        precision,
      );
      const firstOrderInput: KungfuApi.MakeOrderInput = {
        ...makeOrderInput,
        volume: closableVolume,
      };
      const secondOrderInput: KungfuApi.MakeOrderInput = {
        ...makeOrderInput,
        offset: OffsetEnum.Open,
        volume: dealKfDecimalPrecision(volume - closableVolume, precision),
      };
      const flag = await confirmContinueOrderModal(
        t('tradingConfig.close_apart_open_modal', {
          direction,
          oppositeDirection,
          volume,
          closableVolume,
          openVolume,
        }),
        t('tradingConfig.original_plan'),
        t('tradingConfig.beyond_to_open'),
      );

      if (flag !== null) {
        if (flag) {
          return [makeOrderInput];
        } else {
          return [firstOrderInput, secondOrderInput];
        }
      }
    }
  }

  return [makeOrderInput];
}

// 下单
async function handleMakeOrder(): Promise<void> {
  try {
    if (!currentGlobalKfLocation.value) return;

    await formRef.value.validate();
    const makeOrderInput: KungfuApi.MakeOrderInput = await initOrderInputData();
    const flag = await showCloseModal(makeOrderInput);
    if (!flag) return;
    const isContinue = await confirmContinueOrderModal(
      dealFatFingerMessage(makeOrderInput),
    );
    if (isContinue !== null && !isContinue) return;
    const makeOrderInputs = await confirmApartCloseToOpen(makeOrderInput);

    for (let orderInput of makeOrderInputs) {
      const tdProcessId = await confirmOrderPlace(orderInput);
      if (!tdProcessId) {
        continue;
      }
      await placeOrder(orderInput, currentGlobalKfLocation.value, tdProcessId);
    }
    makeOrderRef.value?.focus();
    app?.proxy?.$globalBus.next({
      tag: 'main',
      name: 'click:makeOrder',
      orderInput: makeOrderInput,
    });
  } catch (e) {
    if ((<Error>e).message) {
      error((<Error>e).message);
    }
  }
}

const isShowOrderTriggerConfirmModal = ref<boolean>(false);
const orderTriggerInputResolved = ref<
  Record<string, KungfuApi.KfTradeValueCommonData>
>({});
const orderTriggerInput = ref<KungfuApi.MakeOrderInput>();
const orderTriggerBtnVisible = computed(() => {
  const rootPackageJson = readRootPackageJsonSync();
  if (rootPackageJson?.appConfig?.orderTrigger === false) {
    return false;
  }

  const tdName = currentGlobalKfLocation.value?.group as string;
  const extConfig = extConfigs.value.td[tdName];
  if (
    extConfig &&
    extConfig.orderTrigger[OrderTriggerConfigTypeEnum.MakeOrder]
  ) {
    const { instrument, side } = formState.value;
    if (!instrument) {
      return false;
    }
    const { instrumentType } = transformSearchInstrumentResultToInstrument(
      instrument,
    ) as KungfuApi.InstrumentResolved;
    if (
      instrumentType === InstrumentTypeEnum.future &&
      side !== SideEnum.Exec
    ) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
});

// 预埋
async function handleOrderTrigger() {
  try {
    if (!currentGlobalKfLocation.value) {
      orderTriggerRef.value?.focus();
      return;
    }

    await formRef.value.validate();
    orderTriggerInput.value = await initOrderInputData();

    const { account_id } = formState.value;
    const tdProcessId =
      currentGlobalKfLocation.value?.category === 'td'
        ? getProcessIdByKfLocation(currentGlobalKfLocation.value)
        : `td_${account_id.toString()}`;

    if (processStatusData.value[tdProcessId] !== 'online') {
      error(t('tradingConfig.start_process', { process: tdProcessId }));
      orderTriggerRef.value?.focus();
      return;
    }

    isShowOrderTriggerConfirmModal.value = true;
    const { price_precision } = getPriceTickAndPrecision(
      orderTriggerInput.value.instrument_id,
      orderTriggerInput.value.exchange_id,
    );
    orderTriggerInputResolved.value = dealOrderInputItem(
      orderTriggerInput.value,
      price_precision,
    );
  } catch (e) {
    if ((<Error>e).message) {
      error((<Error>e).message);
    }
  }
}

function handleOrderTriggerConfirm() {
  if (!currentGlobalKfLocation.value) {
    orderTriggerRef.value?.focus();
    return;
  }
  const orderInput: KungfuApi.MakeOrderTriggerInput = {
    ...(orderTriggerInput.value as KungfuApi.MakeOrderInput),
  };

  const { account_id } = formState.value;
  const tdProcessId =
    currentGlobalKfLocation.value?.category === 'td'
      ? getProcessIdByKfLocation(currentGlobalKfLocation.value)
      : `td_${account_id.toString()}`;

  if (processStatusData.value[tdProcessId] !== 'online') {
    error(t('tradingConfig.start_process', { process: tdProcessId }));
    orderTriggerRef.value?.focus();
    return;
  }

  makeOrderByOrderTriggerInput(
    window.watcher,
    orderInput,
    currentGlobalKfLocation.value,
    tdProcessId.toAccountId(),
  )
    .then(() => {
      success();
    })
    .catch((e) => {
      error((<Error>e).message);
    });
  orderTriggerRef.value?.focus();
}

// 展示平仓弹窗
function showCloseModal(
  makeOrderInput: KungfuApi.MakeOrderInput,
): Promise<boolean> {
  if (!currentPosition.value || globalSetting.value?.trade?.close === 0)
    return Promise.resolve(true);

  const closeRange = +globalSetting.value?.trade?.close || 100;

  const { result, relationship } = closeModalConditions(
    closeRange,
    makeOrderInput,
    currentPosition.value?.closable_volume || 0,
  );

  if (result) {
    return confirmModal(
      t('prompt'),
      t('tradingConfig.continue_close_rate', {
        rate: closeRange + '',
        relationship,
      }),
    );
  }

  return Promise.resolve(true);
}

// 触发平仓弹窗条件
function closeModalConditions(
  closeRange: number,
  orderInput: KungfuApi.MakeOrderInput,
  positionVolume: number,
): {
  result: boolean;
  relationship?: string;
} {
  const precision = getPrecisionByInstrumentType(makeOrderInstrumentType.value);
  const makeOrderInput = dealStockOffset(orderInput);
  const { offset } = makeOrderInput;

  if (offset === OffsetEnum.Open) {
    return { result: false };
  }

  const positionVolumeResolved = dealKfDecimalPrecision(
    positionVolume * (closeRange / 100),
    precision,
  );

  if (makeOrderInput.volume === positionVolumeResolved) {
    return {
      result: true,
      relationship: t('tradingConfig.reach'),
    };
  } else if (makeOrderInput.volume > positionVolumeResolved) {
    return {
      result: true,
      relationship: t('tradingConfig.above'),
    };
  } else {
    return {
      result: false,
    };
  }
}

const dealStringToNumber = (tar: string) =>
  Number.isNaN(Number(tar)) ? 0 : Number(tar);

let lastPercentSetVolume = 0;
const handlePercentChange = (target: number) => {
  const { side, offset } = formState.value;
  const { instrumentId, exchangeId, instrumentType } =
    instrumentResolved.value || {};
  let quantityUnit = 0;
  if (instrumentId && exchangeId) {
    const { quantity_unit } = getQuantityUnitAndPrecision(
      instrumentId,
      exchangeId,
    );
    quantityUnit = quantity_unit;
  }

  const curOffset = getResolvedOffset(
    offset,
    side,
    instrumentResolved.value?.instrumentType,
  );

  const targetPercent = target / 100;

  let targetVolume;
  if (curOffset === OffsetEnum.Open) {
    const availMoney = dealStringToNumber(currentAvailMoney.value + '');
    const allVolume = currentPrice.value ? availMoney / currentPrice.value : 0;
    targetVolume = allVolume * targetPercent;
  } else {
    const availPosVolume = dealStringToNumber(currentAvailPosVolume.value);
    targetVolume = availPosVolume * targetPercent;
  }

  formState.value.volume = dealVolumeByInstrumentType(
    targetVolume,
    instrumentType,
    quantityUnit,
  );
  if (formState.value.volume) {
    currentPercent.value = target;
    lastPercentSetVolume = formState.value.volume;
  }
};

watch(
  () => formState.value.volume,
  (newVal) => {
    if (newVal !== lastPercentSetVolume) {
      currentPercent.value = 0;
    }
  },
);
</script>

<template>
  <div class="kf-make-order-dashboard__warp">
    <KfDashboard
      ref="boardRef"
      tabindex="0"
      @boardSizeChange="handleBodySizeChange"
    >
      <template v-slot:title>
        <span v-if="currentGlobalKfLocation">
          <a-tag
            v-if="currentCategoryData"
            :color="currentCategoryData?.color || 'default'"
          >
            {{ currentCategoryData?.name }}
          </a-tag>
          <span class="name" v-if="currentGlobalKfLocation">
            {{ getCurrentGlobalKfLocationId(currentGlobalKfLocation) }}
          </span>
        </span>
      </template>
      <template #header>
        <KfDashboardItem>
          <a-button
            tabindex="-2"
            style="flex: 0"
            size="small"
            @click="handleResetMakeOrderForm"
          >
            {{ $t('tradingConfig.reset_order') }}
          </a-button>
        </KfDashboardItem>
      </template>
      <div class="make-order__wrap">
        <div class="make-order-content">
          <div class="make-order-form__warp">
            <KfConfigSettingsForm
              ref="formRef"
              v-model:formState="formState"
              :configSettings="configSettings"
              :tdList="tdList"
              changeType="add"
              :label-col="LABEL_COL"
              :wrapper-col="WRAPPER_COL"
              :rules="rules"
            ></KfConfigSettingsForm>
            <div class="percent-group__wrap">
              <a-col :span="LABEL_COL - 2"></a-col>
              <a-col :span="LABEL_COL + WRAPPER_COL">
                <a-button
                  v-for="percent in percentList"
                  tabindex="-1"
                  :class="{
                    'percent-button': true,
                    'percent-button-active': currentPercent === percent,
                  }"
                  :key="percent"
                  size="small"
                  ghost
                  @click="
                    currentPercent !== percent && handlePercentChange(percent)
                  "
                >
                  {{ `${percent}%` }}
                </a-button>
              </a-col>
            </div>
            <template v-if="isAccountOrInstrumentConfirmed">
              <div class="make-order-position" tabindex="-1">
                <a-col :span="LABEL_COL - 2"></a-col>
                <a-col :span="WRAPPER_COL">
                  <span class="position-label">
                    {{
                      showAmountOrPosition === 'amount'
                        ? $t('可用资金')
                        : $t('可用仓位')
                    }}
                  </span>
                  <span class="position-value">
                    {{ availablePosOrAmount }}
                  </span>
                </a-col>
              </div>
              <div class="make-order-position" tabindex="-1">
                <a-col :span="LABEL_COL - 2"></a-col>
                <a-col :span="WRAPPER_COL">
                  <span class="position-label">
                    {{
                      isMarginMakeOrder
                        ? $t('交易金额')
                        : isShotable(instrumentResolved?.instrumentType)
                        ? formState.offset === OffsetEnum.Open
                          ? t('保证金占用')
                          : t('保证金返还')
                        : $t('交易金额')
                    }}
                  </span>
                  <span class="position-value">
                    {{ currentTradeAmount }}
                  </span>
                </a-col>
              </div>
              <div class="make-order-position" tabindex="-1">
                <a-col :span="LABEL_COL - 2"></a-col>
                <a-col :span="WRAPPER_COL">
                  <span class="position-label">
                    {{
                      showAmountOrPosition === 'amount'
                        ? $t('剩余资金')
                        : $t('剩余仓位')
                    }}
                  </span>
                  <span class="position-value">
                    {{ leftPosOrAmount }}
                  </span>
                </a-col>
              </div>
            </template>
          </div>
        </div>
        <div class="make-order-btns">
          <a-button
            ref="makeOrderRef"
            class="make-order"
            @click="handleMakeOrder"
          >
            {{ $t('tradingConfig.place_order') }}
          </a-button>
          <a-button
            ref="orderTriggerRef"
            v-if="orderTriggerBtnVisible"
            @click="handleOrderTrigger"
          >
            {{ $t('tradingConfig.order_trigger') }}
          </a-button>
          <a-button ref="apartOrderRef" @click="handleApartOrder">
            {{ $t('tradingConfig.apart_order') }}
          </a-button>
        </div>
      </div>
    </KfDashboard>
    <OrderTriggerConfirmModal
      v-if="isShowOrderTriggerConfirmModal"
      v-model:visible="isShowOrderTriggerConfirmModal"
      :orderTriggerInput="orderTriggerInputResolved"
      @close="
        () => {
          orderTriggerRef && orderTriggerRef.focus();
        }
      "
      @confirm="handleOrderTriggerConfirm"
    ></OrderTriggerConfirmModal>
    <OrderConfirmModal
      v-if="isShowConfirmModal && curOrderType"
      v-model:visible="isShowConfirmModal"
      :curOrderVolume="curOrderVolume"
      :curOrderType="curOrderType"
      @close="
        () => {
          apartOrderRef && apartOrderRef.focus();
        }
      "
      @confirm="handleApartedConfirm"
    ></OrderConfirmModal>
  </div>
</template>
<style lang="less">
.kf-make-order-dashboard__warp {
  width: 100%;
  height: 100%;

  .make-order__wrap {
    height: 100%;
    display: flex;
    justify-content: space-between;
    position: relative;

    .make-order-content {
      flex: 1;
      height: 100%;
      width: calc(100% - 44px);
      margin-right: 44px;
      display: flex;
      flex-direction: column;
      justify-content: flex-start;
    }

    .make-order-form__warp {
      height: 100%;
      padding-top: 16px;
      overflow-y: overlay;

      .ant-form-item {
        margin-bottom: 16px;

        .ant-form-item-explain,
        .ant-form-item-extra {
          min-height: unset;
        }
      }

      .percent-group__wrap {
        margin: auto;
        padding-right: 16px;
        padding-left: 8px;
        box-sizing: border-box;
        display: flex;
        margin-bottom: 12px;

        .ant-col {
          margin: auto;
        }

        .percent-button {
          margin: 0px 8px 8px 0px;
          color: @border-color-base;
          border-color: @border-color-base;
        }

        .percent-button-active {
          color: @primary-color;
          border-color: @primary-color;
        }
      }
    }

    .make-order-position {
      display: flex;
      font-size: 12px;
      color: @text-color;
      padding-top: 4px;

      .position-label {
        padding-right: 8px;
        text-align: left;
      }

      .position-value {
        font-weight: bold;

        &.dash {
          color: @text-color-secondary;
        }
      }

      &:first-child {
        margin-top: 8px;
      }
    }

    .make-order-btns {
      width: 40px;
      height: 100%;
      display: flex;
      flex-direction: column;
      position: absolute;
      right: 0;

      .ant-btn {
        height: 26%;
        text-align: center;
        word-break: break-word;
        word-wrap: unset;
        white-space: normal;
        flex: 1;
        margin-bottom: 8px;

        &:last-child {
          margin-bottom: 0px;
        }
      }

      .make-order {
        height: 72%;
        flex: 4;
      }
    }
  }

  .green {
    color: @green-base !important;
  }

  .red {
    color: @red-base !important;
  }
}

.modal-node {
  .root-node {
    display: flex;
    flex-wrap: nowrap;

    .green {
      color: @green-base !important;
    }

    .red {
      color: @red-base !important;
    }

    .order-number {
      flex: 1;
      margin-top: 10%;
      text-align: center;
    }
  }
}

.ant-modal-confirm-content {
  white-space: pre-wrap;
}
</style>
