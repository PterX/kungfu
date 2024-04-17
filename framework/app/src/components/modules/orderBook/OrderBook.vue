<script setup lang="ts">
import { ExchangeIds } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import {
  dealKfNumber,
  dealKfDecimalPrecision,
  countDecimalPlaces,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { useTriggerMakeOrder } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  computed,
  getCurrentInstance,
  onBeforeUnmount,
  onMounted,
  onDeactivated,
  onActivated,
  ref,
} from 'vue';
import KfBlinkNum from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfBlinkNum.vue';
import {
  SideEnum,
  InstrumentTypeEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import { useQuote } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { getPrecisionByInstrumentType } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

const currentInstrument = ref<KungfuApi.InstrumentResolved | undefined>();
const { getQuoteByInstrument, getLastPricePercent } = useQuote();
const { triggerOrderBookUpdate } = useTriggerMakeOrder();
const app = getCurrentInstance();
const quoteData = computed(() => {
  if (!currentInstrument.value) {
    return null;
  }

  return getQuoteByInstrument(currentInstrument.value);
});

onMounted(() => {
  currentInstrument.value = useGlobalStore().orderBookCurrentInstrument;
});

onActivated(() => {
  const subscription = app?.proxy?.$globalBus.subscribe(
    (data: KfEvent.KfBusEvent) => {
      if (data.tag === 'orderbook') {
        currentInstrument.value = data.instrument;
      }
    },
  );

  onBeforeUnmount(() => {
    subscription?.unsubscribe();
  });

  onDeactivated(() => {
    subscription?.unsubscribe();
  });
});

const precision = computed(() => {
  return getPrecisionByInstrumentType(currentInstrument.value?.instrumentType);
});

const askPrices = computed(() => {
  if (!quoteData.value) {
    return [];
  }

  return dealQuoteAskPidPrices(quoteData.value, 'ask');
});

const bidPrices = computed(() => {
  if (!quoteData.value) {
    return [];
  }

  return dealQuoteAskPidPrices(quoteData.value, 'bid');
});

const limitPrices = computed(() => {
  if (!quoteData.value) {
    return [];
  }

  return [quoteData.value.upper_limit_price, quoteData.value.lower_limit_price];
});

const askVolume = computed(() => {
  if (!quoteData.value) {
    return [];
  }

  return quoteData.value.ask_volume;
});

const bidVolume = computed(() => {
  if (!quoteData.value) {
    return [];
  }

  return quoteData.value.bid_volume;
});

function handleTriggerBuyOrderBookPriceVolume(
  price: number,
  volume: number | bigint,
) {
  if (!currentInstrument.value) {
    return;
  }

  triggerOrderBookUpdate(currentInstrument.value, {
    side: SideEnum.Buy,
    price,
    volume: volume,
  });
}

function handleTriggerSellOrderBookPriceVolume(
  price: number,
  volume: number | bigint,
) {
  if (!currentInstrument.value) {
    return;
  }

  triggerOrderBookUpdate(currentInstrument.value, {
    side: SideEnum.Sell,
    price,
    volume: volume,
  });
}

function dealQuoteAskPidPrices(
  quoteData: KungfuApi.Quote,
  type: 'ask' | 'bid',
) {
  if (quoteData.instrument_type === InstrumentTypeEnum.future) {
    if (currentInstrument.value) {
      let price_tick =
        (
          (window.watcher?.ledger?.Instrument[currentInstrument.value.ukey] ||
            {}) as KungfuApi.Instrument
        ).price_tick ?? 0.001;

      const target_price_tick = type === 'ask' ? +price_tick : -price_tick;
      if (price_tick !== 0) {
        return quoteData[`${type}_price`].reduce((pre, cur, index) => {
          if (
            index === 0 ||
            dealKfDecimalPrecision(
              toLedgalPriceVolume(cur),
              countDecimalPlaces(target_price_tick),
            )
          ) {
            pre.push(
              dealKfDecimalPrecision(
                toLedgalPriceVolume(cur),
                countDecimalPlaces(target_price_tick),
              ),
            );
          } else {
            const prePrice = pre[index - 1];
            pre.push(
              dealKfDecimalPrecision(
                toLedgalPriceVolume(prePrice + target_price_tick),
                countDecimalPlaces(target_price_tick),
              ),
            );
          }

          return pre;
        }, [] as number[]);
      }
    }
  }

  return quoteData[`${type}_price`];
}

function toLedgalPriceVolume(num: number | bigint) {
  num = Number(num);

  if (Number.isNaN(num)) {
    return 0;
  }

  if (num >= Number.MAX_SAFE_INTEGER) {
    return 0;
  }

  if (num < 0) {
    return 0;
  }

  return num;
}
</script>
<template>
  <div class="kf-order-book__warp">
    <div class="level-book">
      <div class="level-row">
        <div class="left_warp"></div>
        <div class="price">
          {{
            limitPrices[0] !== 0 && limitPrices[0] !== undefined
              ? dealKfNumber(toLedgalPriceVolume(limitPrices[0]), precision)
              : '--'
          }}
        </div>
        <div class="limit_price_name">
          {{ t('tradingConfig.up_limit_price') }}
        </div>
      </div>
      <div v-for="(_item, index) in Array(10)" :key="index" class="level-row">
        <div
          class="buy volume"
          @click="
            handleTriggerBuyOrderBookPriceVolume(
              toLedgalPriceVolume(askPrices[9 - index]),
              0,
            )
          "
        ></div>
        <div class="price">
          {{
            dealKfNumber(toLedgalPriceVolume(askPrices[9 - index]), precision)
          }}
        </div>
        <div
          class="sell volume"
          @click="
            handleTriggerSellOrderBookPriceVolume(
              toLedgalPriceVolume(askPrices[9 - index]),
              toLedgalPriceVolume(askVolume[9 - index]),
            )
          "
        >
          {{
            dealKfNumber(toLedgalPriceVolume(askVolume[9 - index]), precision)
          }}
        </div>
      </div>
    </div>
    <div class="instrument-info">
      <div class="info info-item">
        <div class="main">
          {{
            currentInstrument?.instrumentName ||
            currentInstrument?.instrumentId ||
            '--'
          }}
        </div>
        <div class="sub">
          <span>{{ currentInstrument?.instrumentId || '--' }}</span>
          <span>
            {{
              (ExchangeIds[currentInstrument?.exchangeId || ''] || {}).name ||
              '--'
            }}
          </span>
        </div>
      </div>
      <div class="price info-item">
        <div class="main">
          {{
            dealKfNumber(
              getQuoteByInstrument(currentInstrument)?.last_price,
              precision,
            )
          }}
        </div>
        <div class="sub">
          <KfBlinkNum
            blink-type="color"
            mode="compare-zero"
            :num="
              currentInstrument ? getLastPricePercent(currentInstrument) : 0
            "
          ></KfBlinkNum>
        </div>
      </div>
    </div>
    <div class="level-book">
      <div v-for="(_item, index) in Array(10)" :key="index" class="level-row">
        <div
          class="buy volume"
          @click="
            handleTriggerBuyOrderBookPriceVolume(
              toLedgalPriceVolume(bidPrices[index]),
              toLedgalPriceVolume(bidVolume[index]),
            )
          "
        >
          {{ dealKfNumber(toLedgalPriceVolume(bidVolume[index]), precision) }}
        </div>
        <div class="price">
          {{ dealKfNumber(toLedgalPriceVolume(bidPrices[index]), precision) }}
        </div>
        <div
          class="sell volume"
          @click="
            handleTriggerSellOrderBookPriceVolume(
              toLedgalPriceVolume(bidPrices[index]),
              0,
            )
          "
        ></div>
      </div>
      <div class="level-row">
        <div class="left_warp"></div>
        <div class="price">
          {{
            limitPrices[1] !== 0 && limitPrices[1] !== undefined
              ? dealKfNumber(toLedgalPriceVolume(limitPrices[1]), precision)
              : '--'
          }}
        </div>
        <div class="limit_price_name">
          {{ t('tradingConfig.low_limit_price') }}
        </div>
      </div>
    </div>
  </div>
</template>

<style lang="less">
.kf-order-book__warp {
  height: 100%;
  width: calc(100% - 8px);
  display: flex;
  flex-direction: column;
  padding: 8px 8px;
  overflow-y: overlay;

  .level-book {
    flex: 1;
    display: flex;
    flex-direction: column;

    .level-row {
      flex: 1;
      display: flex;
      justify-content: space-between;

      .price,
      .volume,
      .left_warp,
      .limit_price_name {
        flex: 1;
        padding-right: 8px;
        align-items: center;
        font-weight: bold;
        display: flex;
        justify-content: flex-end;
        align-items: center;
      }

      .price {
        flex: 1.5;
        min-width: 120px;
      }

      .volume {
        color: #fff;
        cursor: pointer;
        min-width: 120px;

        &.buy {
          background: @red-base;

          &:hover {
            background: @red-7;
          }
        }

        &.sell {
          background: @green-base;

          &:hover {
            background: @green-8;
          }
        }
      }
    }
  }

  .instrument-info {
    display: flex;
    align-items: center;
    padding: 8px 0px;
    min-width: 360px;

    .info-item {
      flex: 1;
      flex-direction: column;
      justify-content: space-between;
      display: flex;

      &.info {
        text-align: left;
        flex: 1.5;
      }

      &.price {
        text-align: right;
        flex: 1.5;
      }

      .main {
        font-size: 16px;
        font-weight: bold;
      }

      .sub {
        position: relative;
        font-size: 12px;
        height: 25px;
      }
    }
  }
}
</style>
