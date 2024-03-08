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
const currentTradingData = ref<KungfuApi.tradingData>();

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

const tdChildrenLocationIdList = ref<number[]>([]);
const firstRender = ref<boolean>(true);
const isRendering = ref<boolean>(false);
const subscribeNext = ref<boolean>(false);
const stopSubscribe = ref<boolean>(false);

const searchKeyword = ref<string>('');

async function getTradeList(
  tradingData: KungfuApi.tradingData,
): Promise<KungfuApi.TradeResolved[]> {
  let tradeList: KungfuApi.TradeResolved[] = [];
  if (!currentGlobalKfLocation.value) tradeList = [];
  if (currentGlobalKfLocation.value?.category === 'globalPos') {
    const locationId = getIdByKfLocation(currentGlobalKfLocation.value);
    const addTradeResolved = (
      tradeResolved: KungfuApi.OrderResolved | KungfuApi.TradeResolved,
    ) => {
      const instrumentId = `${tradeResolved.exchange_id}_${tradeResolved.instrument_id}`;

      if (instrumentId === locationId) {
        tradeList.push(tradeResolved as KungfuApi.TradeResolved);
      }
      return true;
    };
    await tradingData.tradingDataForEach(
      addTradeResolved,
      'trade',
      'td',
      'common',
    );
  } else if (
    currentGlobalKfLocation.value?.category === 'td' ||
    currentGlobalKfLocation.value?.category === 'strategy'
  ) {
    const locationId = window.watcher.getLocationUID(
      currentGlobalKfLocation.value,
    );
    const indexMap =
      tradingData.trade[currentGlobalKfLocation.value.category][locationId];
    if (indexMap) {
      tradeList = indexMap.getCommonList();
    } else {
      tradeList = [];
    }
  } else if (currentGlobalKfLocation.value?.category === 'tdGroup') {
    tdChildrenLocationIdList.value = [];
    const locationList = (
      currentGlobalKfLocation.value as KungfuApi.KfLocationGroup
    ).children;
    if (locationList) {
      locationList.forEach((location) => {
        const locationId = location.location_uid;
        tdChildrenLocationIdList.value.push(locationId);
      });
    }
    const addTradeResolved = (
      tradeResolved: KungfuApi.OrderResolved | KungfuApi.TradeResolved,
    ) => {
      if (tradeList.length >= DEFAULT_TRADE_LIST_LENGTH) {
        return false;
      } else {
        tradeList.push(tradeResolved as KungfuApi.TradeResolved);
        return true;
      }
    };
    await tradingData.tradingDataForEach(
      addTradeResolved,
      'trade',
      'td',
      'common',
      tdChildrenLocationIdList.value,
    );
  } else {
    tradeList = [];
  }
  return tradeList;
}

async function processTradingData(tradingData: KungfuApi.tradingData) {
  if (isRendering.value) return;
  isRendering.value = true;
  currentTradingData.value = tradingData;

  const tradeList = await getTradeList(tradingData);

  nextTick(() => {
    if (tradeList.length > 0) {
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
        allTrades.value = toRaw(tableData);
        canvasRef.value.getListTable()?.setRecords(tableData);
      } else {
        canvasRef.value.getListTable()?.setRecords([]);
      }
    } else {
      allTrades.value = [];
      canvasRef.value.getListTable()?.setRecords([]);
    }
  });
  isRendering.value = false;
}

const hasData = computed(() => {
  return allTrades.value.length > 0;
});

onActivated(() => {
  const subscription = app?.proxy?.$tradingDataSubject.subscribe(
    async (data) => {
      const { tradingData, update } = data;
      if (historyDate.value) {
        return;
      }

      if (currentGlobalKfLocation.value === null) {
        return;
      }

      if (
        (update || subscribeNext.value || firstRender.value) &&
        !stopSubscribe.value
      ) {
        firstRender.value = false;
        subscribeNext.value = false;
        await processTradingData(tradingData);
      }
    },
  );

  onBeforeUnmount(() => {
    subscription?.unsubscribe();
  });

  onDeactivated(() => {
    firstRender.value = true;
    subscription?.unsubscribe();
  });
});

watch(
  () => currentGlobalKfLocation.value,
  async () => {
    historyDate.value = undefined;
    allTrades.value = [];
    if (currentGlobalKfLocation.value === null || !currentTradingData.value) {
      return;
    }
    stopSubscribe.value = true;

    await processTradingData(currentTradingData.value);
    stopSubscribe.value = false;
  },
  { immediate: true },
);

watch(historyDate, async (newDate) => {
  subscribeNext.value = true;
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

watch(searchKeyword, () => {
  subscribeNext.value = true;
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
