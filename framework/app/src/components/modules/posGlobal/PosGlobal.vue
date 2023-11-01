<script setup lang="ts">
import {
  useDashboardBodySize,
  useTableSearchKeyword,
  useTriggerMakeOrder,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  computed,
  getCurrentInstance,
  onBeforeUnmount,
  onMounted,
  ref,
  toRaw,
} from 'vue';
import { storeToRefs } from 'pinia';
import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import KfBlinkNum from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfBlinkNum.vue';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import { categoryRegisterConfig, getColumns } from './config';
import { dealKfPrice } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  dealDirection,
  dealCurrency,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import {
  LedgerCategoryEnum,
  SideEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import { ExchangeIds } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import {
  getInstrumentByInstrumentPair,
  useCurrentGlobalKfLocation,
  useInstruments,
  useActiveInstruments,
  useQuote,
  useDealDataWithCaches,
  showTradingDataDetail,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { dealPosition } from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';
import {
  getPosClosableVolumeByOffset,
  resolveTriggerOffset,
} from '../pos/utils';
import { getKfGlobalSettings } from '@kungfu-trader/kungfu-js-api/config/globalSettings';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;

globalThis.HookKeeper.getHooks().dealTradingData.register(
  {
    category: categoryRegisterConfig.category,
    group: '*',
    name: '*',
    mode: '*',
  },
  categoryRegisterConfig,
);

const app = getCurrentInstance();
const pos = ref<KungfuApi.PositionResolved[]>([]);
const { handleBodySizeChange } = useDashboardBodySize();
const { searchKeyword, tableData } =
  useTableSearchKeyword<KungfuApi.PositionResolved>(pos, [
    'instrument_id_resolved',
    'instrument_id',
    'exchange_id',
    'direction',
  ]);

const {
  currentGlobalKfLocation,
  dealRowClassName,
  setCurrentGlobalKfLocation,
} = useCurrentGlobalKfLocation(window.watcher);
const { instruments } = useInstruments();
const { getPositionLastPrice } = useQuote();
const { triggerOrderBook, triggerMakeOrder } = useTriggerMakeOrder();
const {
  getInstrumentCurrencyByIds,
  getPriceTickAndPrecision,
  getInstrumentCurrency,
} = useActiveInstruments();
const { dealDataWithCache } = useDealDataWithCaches<
  KungfuApi.Position,
  KungfuApi.PositionResolved
>(['uid_key', 'update_time']);
const { globalSetting } = storeToRefs(useGlobalStore());

const lastPriceSorter = (a: KungfuApi.Position, b: KungfuApi.Position) => {
  return getPositionLastPrice(a) - getPositionLastPrice(b);
};
const columns = computed(() => {
  const kfGlobalSettings = getKfGlobalSettings();
  const tradeSettings = kfGlobalSettings.filter(
    (item) => item.key === 'trade',
  )[0];
  const posTableColumnsOptions = tradeSettings.config
    .filter((item) => item.key === 'posTableColumns')[0]
    .options?.map((item) => item.value);
  const selectedOptions: string[] = globalSetting.value?.trade?.posTableColumns;
  if (!posTableColumnsOptions || !selectedOptions)
    return getColumns(lastPriceSorter);
  const notSelectedOptions = posTableColumnsOptions.filter((item) => {
    return !selectedOptions.includes(item as string);
  });

  const columnsConfig = getColumns(lastPriceSorter);

  return columnsConfig.filter((item) => {
    return !notSelectedOptions.includes(item.dataIndex);
  });
});

onMounted(() => {
  if (app?.proxy) {
    const subscription = app.proxy.$tradingDataSubject.subscribe(
      (watcher: KungfuApi.Watcher) => {
        setTimeout(() => {
          const positions = watcher.ledger.Position.nofilter(
            'volume',
            BigInt(0),
          )
            .filter('ledger_category', LedgerCategoryEnum.td)
            .list();

          pos.value = toRaw(
            buildGlobalPositions(positions).map((position) => {
              const { price_precision } = getPriceTickAndPrecision(
                position.instrument_id,
                position.exchange_id,
              );
              const currency = getInstrumentCurrency(
                position.instrument_id,
                position.exchange_id,
              );

              return dealDataWithCache(
                position,
                () => dealPosition(watcher, position, price_precision),
                { currency },
              );
            }),
          );
        });
      },
    );

    onBeforeUnmount(() => {
      subscription.unsubscribe();
    });
  }
});

type PosStat = Record<string, KungfuApi.Position & { id: string }>;

function buildGlobalPositions(
  positions: KungfuApi.Position[],
): KungfuApi.Position[] {
  const posStatData: PosStat = positions.reduce((posStat, pos) => {
    const id = `${pos.instrument_id}_${pos.exchange_id}_${pos.direction}`;
    if (!posStat[id]) {
      posStat[id] = Object.assign({}, pos, { id, uid_key: pos.uid_key });
    } else {
      const prePosStat = posStat[id];
      const {
        avg_open_price,
        volume,
        yesterday_volume,
        unrealized_pnl,
        update_time,
        static_yesterday,
        open_volume,
      } = prePosStat;
      posStat[id].yesterday_volume = yesterday_volume + pos.yesterday_volume;
      posStat[id].volume = volume + pos.volume;
      posStat[id].static_yesterday = static_yesterday + pos.static_yesterday;
      posStat[id].open_volume = open_volume + pos.open_volume;
      posStat[id].avg_open_price =
        (avg_open_price * Number(volume) +
          pos.avg_open_price * Number(pos.volume)) /
        (Number(volume) + Number(pos.volume));
      posStat[id].unrealized_pnl = unrealized_pnl + pos.unrealized_pnl;
      posStat[id].update_time =
        update_time > pos.update_time ? update_time : pos.update_time;
    }
    return posStat;
  }, {} as PosStat);

  return Object.values(posStatData);
}

function dealRowClassNameResolved(row: KungfuApi.PositionResolved) {
  const locationResolved: KungfuApi.KfExtraLocation = {
    category: categoryRegisterConfig.category,
    group: row.exchange_id,
    name: row.instrument_id,
    mode: 'live',
    direction: row.direction,
  };

  const classNameResolved = dealRowClassName(locationResolved);

  if (
    classNameResolved &&
    (currentGlobalKfLocation.value as KungfuApi.KfExtraLocation)?.direction ===
      row.direction
  ) {
    return classNameResolved;
  }

  return '';
}

function handleClickRow(data: {
  event: MouseEvent;
  row: KungfuApi.PositionResolved;
  column: KfTradingDataTableHeaderConfig;
}) {
  const locationResolved: KungfuApi.KfExtraLocation = {
    category: categoryRegisterConfig.category,
    group: data.row.exchange_id,
    name: data.row.instrument_id,
    mode: 'live',
    direction: data.row.direction,
  };

  setCurrentGlobalKfLocation(locationResolved);
  tiggerOrderBookAndMakeOrder(data.row);
}

function tiggerOrderBookAndMakeOrder(record: KungfuApi.PositionResolved) {
  const { instrument_id, instrument_type, exchange_id } = record;
  const ensuredInstrument: KungfuApi.InstrumentResolved =
    getInstrumentByInstrumentPair(
      {
        instrument_id,
        instrument_type,
        exchange_id,
      },
      instruments.value,
    );

  const offset = resolveTriggerOffset(record);
  triggerOrderBook(ensuredInstrument);
  const extraOrderInput: ExtraOrderInput = {
    side: record.direction === 0 ? SideEnum.Sell : SideEnum.Buy,
    offset,
    volume: getPosClosableVolumeByOffset(record, offset),

    price: getPositionLastPrice(record) || 0,
  };
  triggerMakeOrder(ensuredInstrument, extraOrderInput);
}

function handleShowTradingDataDetail({
  row,
}: {
  event: MouseEvent;
  row: KungfuApi.PositionResolved;
}) {
  showTradingDataDetail(row, t('posGlobalConfig.pos_detail_header'), [
    'account_id_resolved',
  ]);
}
</script>
<template>
  <div class="kf-position-global__warp kf-translateZ">
    <KfDashboard @boardSizeChange="handleBodySizeChange">
      <template #header>
        <KfDashboardItem>
          <a-input-search
            v-model:value="searchKeyword"
            :placeholder="$t('keyword_input')"
            style="width: 120px"
          />
        </KfDashboardItem>
      </template>
      <KfTradingDataTable
        class="kf-ant-table"
        key-field="uid_key"
        :columns="columns"
        :data-source="tableData"
        :item-size="28"
        :custom-row-class="dealRowClassNameResolved"
        @clickCell="handleClickRow"
        @rightClickRow="handleShowTradingDataDetail"
      >
        <template
          #default="{
            column,
            item,
          }: {
            column: KfTradingDataTableHeaderConfig,
            item: KungfuApi.PositionResolved,
          }"
        >
          <template v-if="column.dataIndex === 'instrument_id'">
            <span>
              {{ item.instrument_id }}
              {{ ExchangeIds[item.exchange_id].name }}
              <span
                v-if="globalSetting?.currency?.instrumentCurrency"
                style="color: #faad14"
              >
                {{
                  dealCurrency(
                    getInstrumentCurrencyByIds(
                      item.instrument_id,
                      item.exchange_id,
                    ),
                  ).name
                }}
              </span>
            </span>
          </template>
          <template v-else-if="column.dataIndex === 'direction'">
            <span :class="`color-${dealDirection(item.direction).color}`">
              {{ dealDirection(item.direction).name }}
            </span>
          </template>
          <template v-else-if="column.dataIndex === 'static_yesterday'">
            <KfBlinkNum
              :num="Number(item.static_yesterday).kfToFixed(0)"
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'open_volume'">
            <KfBlinkNum
              :num="Number(item.open_volume).kfToFixed(0)"
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'close_volume'">
            <KfBlinkNum
              :num="
                Number(
                  item.open_volume + item.static_yesterday - item.volume,
                ).kfToFixed(0)
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'yesterday_volume'">
            <KfBlinkNum
              :num="Number(item.yesterday_volume).kfToFixed(0)"
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'today_volume'">
            <KfBlinkNum
              :num="Number(item.volume - item.yesterday_volume).kfToFixed(0)"
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'volume'">
            <KfBlinkNum :num="Number(item.volume).kfToFixed(0)"></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'avg_open_price_resolved'">
            <KfBlinkNum :num="item.avg_open_price_resolved"></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'last_price_resolved'">
            <KfBlinkNum
              :num="
                dealKfPrice(
                  getPositionLastPrice(item, 'last_price_resolved'),
                  item.price_precision,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'unrealized_pnl_resolved'">
            <KfBlinkNum
              mode="compare-zero"
              :num="item.unrealized_pnl_resolved"
            ></KfBlinkNum>
          </template>
        </template>
      </KfTradingDataTable>
    </KfDashboard>
  </div>
</template>
<style lang="less">
.kf-position-global__warp {
  height: 100%;
}
</style>
