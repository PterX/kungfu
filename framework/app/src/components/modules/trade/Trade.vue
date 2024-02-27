<script setup lang="ts">
import {
  delayMilliSeconds,
  getIdByKfLocation,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  dealOffset,
  dealSide,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { useActiveInstruments } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';

import {
  messagePrompt,
  searchByKeyword,
  useDashboardBodySize,
  useDownloadHistoryTradingData,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import KfCanvasTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfCanvasTradingDataTable.vue';

import {
  DownloadOutlined,
  LoadingOutlined,
  CalendarOutlined,
  PieChartOutlined,
} from '@ant-design/icons-vue';
import { VTable } from '@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable';

import {
  computed,
  getCurrentInstance,
  onBeforeUnmount,
  onActivated,
  onDeactivated,
  ref,
  toRaw,
  watch,
  nextTick,
} from 'vue';
import {
  dealTrade,
  getKungfuHistoryData,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { getColumns } from './config';
import type { Dayjs } from 'dayjs';
import {
  showTradingDataDetail,
  useCurrentGlobalKfLocation,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import TradeStatisticModal from './TradeStatisticModal.vue';
import { HistoryDateEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const DEFAULT_TRADE_LIST_LENGTH = 50000;

const { t } = VueI18n.global;
const app = getCurrentInstance();
const { getPriceTickAndPrecision } = useActiveInstruments();
const { handleBodySizeChange } = useDashboardBodySize();
const allTrades = ref<KungfuApi.TradeResolved[]>([]);
const currentTradingDataObject = ref<KungfuApi.TradingDataObject>();
const tradeIndexMapList = ref<
  KungfuApi.KfDynamicTradingDataIndexedMap<string, KungfuApi.TradeResolved>[]
>([]);

const canvasRef = ref();
const historyDate = ref<Dayjs>();
const historyDataLoading = ref<boolean>();

const {
  currentGlobalKfLocation,
  currentCategoryData,
  getCurrentGlobalKfLocationId,
} = useCurrentGlobalKfLocation(window.watcher);

const { handleDownload } = useDownloadHistoryTradingData();
const statisticModalVisible = ref<boolean>(false);

const columns = computed(() => {
  if (!currentGlobalKfLocation.value) {
    return getColumns(
      {
        category: 'td',
        group: '*',
        name: '*',
        mode: '*',
      },
      !!historyDate.value,
    );
  }

  return getColumns(currentGlobalKfLocation.value, !!historyDate.value);
});

const searchKeyword = ref<string>('');

function getTradeIndexMapList(tradingDataObject: KungfuApi.TradingDataObject) {
  tradeIndexMapList.value = [];
  if (!currentGlobalKfLocation.value) tradeIndexMapList.value = [];
  if (currentGlobalKfLocation.value?.category === 'globalPos') {
    const locationId = getIdByKfLocation(currentGlobalKfLocation.value);
    const indexMap = tradingDataObject.trade.position[locationId];
    if (indexMap) {
      tradeIndexMapList.value.push(indexMap);
    } else {
      tradeIndexMapList.value = [];
    }
  } else if (
    currentGlobalKfLocation.value?.category === 'td' ||
    currentGlobalKfLocation.value?.category === 'strategy'
  ) {
    const locationId = window.watcher.getLocationUID(
      currentGlobalKfLocation.value,
    );
    const indexMap =
      tradingDataObject.trade[currentGlobalKfLocation.value.category][
        locationId
      ];
    if (indexMap) {
      tradeIndexMapList.value.push(indexMap);
    } else {
      tradeIndexMapList.value = [];
    }
  } else if (currentGlobalKfLocation.value?.category === 'tdGroup') {
    const locationList = (
      currentGlobalKfLocation.value as KungfuApi.KfLocationGroup
    ).children;
    if (locationList) {
      locationList.forEach((location) => {
        const locationId = location.location_uid;
        const indexMap = tradingDataObject.trade.td[locationId];
        if (indexMap) {
          tradeIndexMapList.value.push(indexMap);
        }
      });
    }
  } else {
    tradeIndexMapList.value = [];
  }
  return tradeIndexMapList.value;
}
function getTradeListForIndexMap(
  tradeIndexMapList: KungfuApi.KfDynamicTradingDataIndexedMap<
    string,
    KungfuApi.TradeResolved
  >[],
) {
  let tradeList: KungfuApi.TradeResolved[] = [];
  if (tradeIndexMapList.length === 1) {
    const compare = (a, b) => Number(b.trade_time) - Number(a.trade_time);

    tradeList = tradeIndexMapList[0]
      .getCommonList()
      .sort((a, b) => compare(a, b));
  } else {
    const listMap: Record<number, KungfuApi.TradeResolved[]> = {};
    let everyLatestTradeResolved = tradeIndexMapList.map(
      (tradeIndexMap, index) => {
        const list = tradeIndexMap.getCommonList();
        let firstTrade = list[0] as KungfuApi.TradeResolved;
        if (!firstTrade) return;
        listMap[index] = list;
        return { trade: firstTrade, index, position: 0 };
      },
    );

    const compare = (a, b) =>
      Number(b.trade.trade_time) - Number(a.trade.trade_time);
    while (
      everyLatestTradeResolved.length > 0 &&
      tradeList.length < DEFAULT_TRADE_LIST_LENGTH
    ) {
      everyLatestTradeResolved.sort((a, b) => compare(a, b));
      let maxItem = everyLatestTradeResolved.shift();
      if (!maxItem) break;
      tradeList.push(maxItem.trade);

      let nextPosition = maxItem.position + 1;
      let nextTrade = listMap[maxItem.index][nextPosition];
      if (nextTrade) {
        everyLatestTradeResolved.push({
          trade: nextTrade,
          index: maxItem.index,
          position: nextPosition,
        });
      }
    }
  }
  return tradeList;
}

function processTradingData(tradingDataObject: KungfuApi.TradingDataObject) {
  currentTradingDataObject.value = tradingDataObject;

  const indexMapList = getTradeIndexMapList(tradingDataObject);

  nextTick(() => {
    if (indexMapList.length > 0) {
      const tradeList = getTradeListForIndexMap(indexMapList);
      const tableData = searchByKeyword(
        searchKeyword.value,
        tradeList,
        [
          'order_id',
          'trade_id',
          'instrument_id',
          'side',
          'offset',
          'exchange_id',
          'source_uname',
          'dest_uname',
        ],
        {
          side: (item) => dealSide(Number(item)).name,
          offset: (item) => dealOffset(Number(item)).name,
        },
      );
      if (tradeList.length) {
        canvasRef.value.getListTable()?.setRecords(tableData);
        allTrades.value = toRaw(tableData);
      } else {
        canvasRef.value.getListTable()?.setRecords([]);
      }
    } else {
      canvasRef.value.getListTable()?.setRecords([]);
    }
  });
}

const hasData = computed(() => {
  return allTrades.value.length > 0;
});

onActivated(() => {
  const subscription = app?.proxy?.$tradingDataSubject.subscribe((data) => {
    const { tradingDataObject } = data;
    if (historyDate.value) {
      return;
    }

    if (currentGlobalKfLocation.value === null) {
      return;
    }

    processTradingData(tradingDataObject);
  });

  onBeforeUnmount(() => {
    subscription?.unsubscribe();
  });

  onDeactivated(() => {
    subscription?.unsubscribe();
  });
});

watch(currentGlobalKfLocation, () => {
  historyDate.value = undefined;
  allTrades.value = [];
  if (
    currentGlobalKfLocation.value === null ||
    !currentTradingDataObject.value
  ) {
    return;
  }
  processTradingData(currentTradingDataObject.value);
});

watch(historyDate, async (newDate) => {
  if (!newDate) {
    return;
  }

  if (!currentGlobalKfLocation.value) return;

  allTrades.value = [];
  historyDataLoading.value = true;
  delayMilliSeconds(500)
    .then(() =>
      getKungfuHistoryData(
        window.watcher,
        newDate.format(),
        HistoryDateEnum.naturalDate,
        'Trade',
        currentGlobalKfLocation.value as KungfuApi.KfLocation,
      ),
    )
    .then((historyData) => {
      if (!historyData) return;

      const { tradingData } = historyData;

      const tradesResolved =
        globalThis.HookKeeper.getHooks().dealTradingData.trigger(
          window.watcher,
          currentGlobalKfLocation.value,
          tradingData.Trade,
          'trade',
        ) as KungfuApi.Trade[];

      const tempAllTrades = toRaw(
        tradesResolved.map((item) => {
          const { price_precision } = getPriceTickAndPrecision(
            item.instrument_id,
            item.exchange_id,
          );

          return toRaw(
            dealTrade(
              window.watcher,
              item,
              tradingData.OrderStat,
              true,
              price_precision,
            ),
          );
        }),
      );

      allTrades.value = tempAllTrades;
      canvasRef.value.getListTable()?.setRecords(allTrades.value);
    })
    .catch((err) => {
      if (err.message === 'database_locked') {
        messagePrompt().error(t('export_database_locked'));
      } else {
        console.error(err.message);
      }
    })
    .finally(() => {
      historyDataLoading.value = false;
    });
});

function handleShowTradingDataDetail(args: VTable.MousePointerCellEvent) {
  const { originData } = args;
  if (!originData) return;
  showTradingDataDetail(originData as KungfuApi.TradeResolved, '成交');
}
</script>
<template>
  <div class="kf-trades__warp kf-translateZ">
    <KfDashboard @boardSizeChange="handleBodySizeChange">
      <template #title>
        <span v-if="currentGlobalKfLocation">
          <a-tag
            v-if="currentCategoryData"
            :color="currentCategoryData?.color || 'default'"
          >
            {{ currentCategoryData?.name }}
          </a-tag>
          <span v-if="currentGlobalKfLocation" class="name">
            {{ getCurrentGlobalKfLocationId(currentGlobalKfLocation) }}
          </span>
        </span>
      </template>
      <template #header>
        <KfDashboardItem>
          <a-input-search
            v-model:value="searchKeyword"
            :placeholder="$t('keyword_input')"
            style="width: 120px"
          />
        </KfDashboardItem>
        <KfDashboardItem>
          <a-date-picker
            v-model:value="historyDate"
            :disabled="historyDataLoading"
          >
            <template #suffixIcon>
              <LoadingOutlined v-if="historyDataLoading" />
              <CalendarOutlined v-else />
            </template>
          </a-date-picker>
        </KfDashboardItem>
        <KfDashboardItem>
          <a-button size="small" @click="statisticModalVisible = true">
            <template #icon>
              <PieChartOutlined style="font-size: 14px"></PieChartOutlined>
            </template>
          </a-button>
        </KfDashboardItem>
        <KfDashboardItem>
          <a-button
            size="small"
            @click="handleDownload('Trade', currentGlobalKfLocation)"
          >
            <template #icon>
              <DownloadOutlined style="font-size: 14px" />
            </template>
          </a-button>
        </KfDashboardItem>
      </template>
      <KfCanvasTradingDataTable
        ref="canvasRef"
        :columns="columns"
        :hasData="hasData"
        @right-click-row="handleShowTradingDataDetail"
      />
    </KfDashboard>
    <TradeStatisticModal
      v-if="statisticModalVisible"
      v-model:visible="statisticModalVisible"
      :trades="allTrades"
      :history-date="historyDate"
    ></TradeStatisticModal>
  </div>
</template>
<style lang="less">
.kf-trades__warp {
  width: 100%;
  height: 100%;

  .kf-table__warp {
    width: 100%;
    height: 100%;

    .kf-trading-data-table {
      width: 100%;
      height: 100%;
    }
  }
}
</style>
