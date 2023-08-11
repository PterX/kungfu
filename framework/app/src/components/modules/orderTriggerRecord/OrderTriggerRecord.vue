<script setup lang="ts">
import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';

import {
  confirmModal,
  messagePrompt,
  useDashboardBodySize,
  useTableSearchKeyword,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { getColumns } from './config';
import {
  dealSide,
  dealOffset,
  isShotable,
  getProcessIdByKfLocation,
  transformSearchInstrumentResultToInstrument,
  getIdByKfLocation,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';

import {
  showTradingDataDetail,
  useCurrentGlobalKfLocation,
  useExtConfigsRelated,
  useProcessStatusDetailData,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import {
  getCurrentInstance,
  onBeforeUnmount,
  onMounted,
  ref,
  watch,
} from 'vue';
import {
  dealOrderTrigger,
  longfist,
  kfOrderTrigger,
  kfRefreshOrderTrigger,
  hashInstrumentUKey,
  kfCancelOriderTrigger,
  kfCancelAllOrdersTrigger,
} from '@kungfu-trader/kungfu-js-api/kungfu';
import KfSetByConfigModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfSetByConfigModal.vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { getModalSettings } from './config';
import {
  InstrumentTypeEnum,
  OffsetEnum,
  OrderTriggerParkedTypeEnum,
  SideEnum,
  TimeConditionEnum,
  OrderTriggerTypeEnum,
  OrderTriggerStatusEnum,
  OrderTriggerFlag,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import { ReloadOutlined, LoadingOutlined } from '@ant-design/icons-vue';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';
import {
  OrderCancelledStatus,
  OrderTriggerCancelStatus,
} from '@kungfu-trader/kungfu-js-api/config/tradingConfig';

interface csvOrderInput {
  limit_price: string;
  volume: string;
  price_type: string;
  side: string;
  offset: string;
  instrument_id: string;
  exchange_id: string;
  instrument: string;
}

const { t } = VueI18n.global;
const { error, success } = messagePrompt();

const { dashboardBodyHeight, handleBodySizeChange } = useDashboardBodySize();
const { processStatusData } = useProcessStatusDetailData();
const app = getCurrentInstance();
const { extConfigs } = useExtConfigsRelated();
const store = useGlobalStore();

const {
  currentGlobalKfLocation,
  currentCategoryData,
  getCurrentGlobalKfLocationId,
} = useCurrentGlobalKfLocation(window.watcher);
const columns = getColumns();

const selectedRowKeys = ref<number[]>([]);
const selectedRows = ref<KungfuApi.OrderTriggerResolved[]>([]);
const tableDataResolved = ref<KungfuApi.OrderTriggerResolved[]>([]);
const batchOrderTriggerVisble = ref<boolean>(false);
const batchOrderTriggerConfigPayload = ref<KungfuApi.SetKfConfigPayload>({
  type: 'custom',
  title: 'orderTrigger',
  config: {} as KungfuApi.KfExtConfig,
});

const getResolvedOffset = (
  offset: OffsetEnum,
  side: SideEnum,
  instrumentType: InstrumentTypeEnum,
) => {
  if (isShotable(instrumentType)) {
    if (offset !== undefined) {
      return offset;
    }
  }
  return side === 0 ? 0 : 1;
};

watch(
  () => currentGlobalKfLocation.value,
  () => {
    selectedRowKeys.value = [];
    selectedRows.value = [];
  },
);

onMounted(() => {
  if (app?.proxy) {
    const subscription = app.proxy.$tradingDataSubject.subscribe(
      (watcher: KungfuApi.Watcher) => {
        if (!currentGlobalKfLocation.value) return;
        const source = watcher.getLocationUID(currentGlobalKfLocation.value);
        const orderTriggerData = (
          window.watcher.ledger[
            'OrderTrigger'
          ] as KungfuApi.DataTable<KungfuApi.OrderTrigger>
        )
          .filter('source', source)
          .list();

        tableDataResolved.value = orderTriggerData.map((item, index) => {
          return dealOrderTrigger(window.watcher, item, false, 4, index);
        });
      },
    );

    onBeforeUnmount(() => {
      subscription.unsubscribe();
    });
  }
});

const { searchKeyword, tableData } =
  useTableSearchKeyword<KungfuApi.OrderTriggerResolved>(tableDataResolved, [
    'update_time_resolved',
    'instrument_id',
    'limit_price_resolved',
    'parked_type',
    'volume',
    'time_condition',
    'dest_uname',
    'status',
  ]);

const customRow = (row: KungfuApi.OrderTriggerResolved) => {
  return {
    onMousedown: (e: MouseEvent) => {
      if (e.button !== 2) return;
      showTradingDataDetail(row, t('tradingConfig.order_trigger'));
    },
  };
};

function handleBatchModal() {
  if (
    !currentGlobalKfLocation.value ||
    currentGlobalKfLocation.value.category !== 'td'
  )
    return;

  const tdProcessId = getProcessIdByKfLocation(currentGlobalKfLocation.value);
  if (processStatusData.value[tdProcessId] !== 'online') {
    error(t('tradingConfig.start_process', { process: tdProcessId }));
    return;
  }

  const tdName = currentGlobalKfLocation.value?.group as string;
  const extConfig = extConfigs.value.td[tdName];
  if (extConfig && !extConfig.orderTrigger[OrderTriggerTypeEnum.MakeOrder]) {
    error(
      t('tradingConfig.order_trigger_td_error', {
        tdName,
      }),
    );
    return;
  }

  batchOrderTriggerConfigPayload.value.title = t(
    'tradingConfig.batch_order_trigger',
  );
  batchOrderTriggerConfigPayload.value.config = {
    type: [],
    name: t('tradingConfig.batch_order_trigger'),
    category: 'orderTrigger',
    key: 'orderTrigger',
    extPath: '',
    settings: getModalSettings(),
  };

  batchOrderTriggerVisble.value = true;
}

function handleConfirmBatchOrderTrigger(csvData: csvOrderInput[]) {
  if (!currentGlobalKfLocation.value) return;

  const emptyRow = csvData.filter((item) => !item.instrument);
  if (emptyRow.length > 0) {
    error(t('tradingConfig.empty_csv_order'));
    return;
  }

  const tdProcessId = getProcessIdByKfLocation(currentGlobalKfLocation.value);
  if (processStatusData.value[tdProcessId] !== 'online') {
    error(t('tradingConfig.start_process', { process: tdProcessId }));
    return;
  }

  const notFutureRow: number[] = [];
  const orderTriggerInputs = csvData
    .map((item: csvOrderInput, index) => {
      const { instrumentType, exchangeId, instrumentId } =
        transformSearchInstrumentResultToInstrument(
          item.instrument,
        ) as KungfuApi.InstrumentResolved;

      const ukey = hashInstrumentUKey(instrumentId, exchangeId);
      const instrumentResolved =
        (window.watcher as KungfuApi.Watcher)?.ledger?.Instrument?.[ukey] ||
        store.instrumentsMap[ukey];

      if (!instrumentResolved || instrumentType !== InstrumentTypeEnum.future) {
        notFutureRow.push(index + 1);
        return null;
      }

      const { limit_price, volume, price_type, side, offset } = item;

      const orderTriggerInput: KungfuApi.MakeOrderTriggerInput = {
        ...longfist.types.OrderInput(),
        instrument_id: instrumentId,
        instrument_type: +instrumentType,
        exchange_id: exchangeId,
        limit_price: +limit_price,
        volume: +volume,
        price_type: +price_type,
        side: +side,
        offset: getResolvedOffset(+offset, +side, instrumentType),
        parked_type: OrderTriggerParkedTypeEnum.Server,
        time_condition: TimeConditionEnum.GFA,
      };

      return orderTriggerInput;
    })
    .filter((item) => item !== null);

  if (notFutureRow.length > 0) {
    const rowStr = notFutureRow.join(', ');
    error(
      t('tradingConfig.order_trigger_not_future', {
        rowStr,
      }),
    );
    return;
  }

  const orderTriggerPromises = orderTriggerInputs.map((orderTriggerInput) => {
    return kfOrderTrigger(
      window.watcher,
      orderTriggerInput as KungfuApi.MakeOrderTriggerInput,
      currentGlobalKfLocation.value as KungfuApi.KfLocation,
    );
  });

  if (orderTriggerPromises.length === 0) {
    return;
  }

  Promise.allSettled(orderTriggerPromises).then(() => {
    success();
  });
}

function handleRequestOrderTrigger() {
  const msgType = Object.keys(longfist.msgTypes).filter((msgType) => {
    if (longfist.msgTypes[msgType] === 'OrderTriggerRequest') {
      return msgType;
    }
    return false;
  });

  kfRefreshOrderTrigger(
    window.watcher,
    Number(msgType[0]),
    currentGlobalKfLocation.value as KungfuApi.KfLocation,
  )
    .then(() => {
      success(t('orderTriggerConfig.order_trigger_request_success'));
    })
    .catch((err: Error) => {
      error(err.message);
    })
    .finally(() => {
      selectedRowKeys.value = [];
      selectedRows.value = [];
    });
}

function onSelectChange(
  rowsKeys: number[],
  rows: KungfuApi.OrderTriggerResolved[],
) {
  selectedRowKeys.value = rowsKeys;
  selectedRows.value = rows;
}

function getCheckboxProps(record) {
  return {
    disabled: !orderTriggerCanBeCancel(record),
  };
}

function handleCancelOrderTrigger(
  orderTrigger: KungfuApi.OrderTriggerResolved,
) {
  if (!currentGlobalKfLocation.value || !window.watcher) {
    error();
    return;
  }

  if (!orderTriggerCanBeCancel(orderTrigger)) {
    return;
  }

  const { order_id } = orderTrigger;
  const curOrder = (window.watcher as KungfuApi.Watcher).ledger.Order.filter(
    'order_id',
    order_id,
  ).list()[0];
  if (curOrder && OrderCancelledStatus.includes(curOrder.status)) {
    error(t('orderTriggerConfig.order_finished'));
    handleRequestOrderTrigger();
    return;
  }

  kfCancelOriderTrigger(
    window.watcher,
    orderTrigger,
    currentGlobalKfLocation.value,
  )
    .then(() => {
      success();
    })
    .catch(() => {
      error();
    });
}

function handleCancelAllOrderTrigger() {
  if (!currentGlobalKfLocation.value || !window.watcher) {
    error();
    return;
  }

  const tdProcessId = getProcessIdByKfLocation(currentGlobalKfLocation.value);
  if (processStatusData.value[tdProcessId] !== 'online') {
    error(t('tradingConfig.start_process', { process: tdProcessId }));
    return;
  }

  const orderTriggers = selectedRows.value.filter((item) => {
    return orderTriggerCanBeCancel(item);
  });
  if (orderTriggers.length === 0) return;

  const cancelOrderTriggers = orderTriggers.filter((order) => {
    return order.action_flag === OrderTriggerFlag.TriggerCancel;
  });
  const insertOrderTriggers = orderTriggers.filter((order) => {
    return order.action_flag === OrderTriggerFlag.TriggerInsert;
  });
  const unfinishedOrderTriggers = cancelOrderTriggers.filter((item) => {
    const { order_id } = item;
    const curOrder = (window.watcher as KungfuApi.Watcher).ledger.Order.filter(
      'order_id',
      order_id,
    ).list()[0];
    if (!OrderCancelledStatus.includes(curOrder.status)) {
      return true;
    }
    return false;
  });

  const name = getIdByKfLocation(currentGlobalKfLocation.value);

  confirmModal(
    t('orderTriggerConfig.confirm_cancel_all'),
    `${t('orderTriggerConfig.confirm')}${
      currentCategoryData.value?.name
    } ${name} ${t('orderTriggerConfig.cancel_all')}`,
  )
    .then((flag) => {
      if (!flag || !currentGlobalKfLocation.value || !window.watcher) {
        return;
      }

      return kfCancelAllOrdersTrigger(
        window.watcher,
        [...insertOrderTriggers, ...unfinishedOrderTriggers],
        currentGlobalKfLocation.value,
      )
        .then(() => {
          success();
        })
        .catch((err) => {
          error(err.message);
        });
    })
    .finally(() => {
      handleRequestOrderTrigger();
    });
}

function orderTriggerCanBeCancel(record: KungfuApi.OrderTriggerResolved) {
  return OrderTriggerCancelStatus.includes(record.status);
}
</script>
<template>
  <div class="kf-market-data__warp">
    <KfDashboard @board-size-change="handleBodySizeChange">
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
          <a-button size="small" @click="handleRequestOrderTrigger">
            <template #icon>
              <ReloadOutlined style="font-size: 14px" />
            </template>
          </a-button>
        </KfDashboardItem>
        <KfDashboardItem>
          <a-button
            size="small"
            type="primary"
            danger
            @click="handleCancelAllOrderTrigger"
          >
            {{ $t('orderTriggerConfig.cancel_all') }}
          </a-button>
        </KfDashboardItem>
        <KfDashboardItem>
          <a-button size="small" type="primary" @click="handleBatchModal">
            {{ $t('tradingConfig.batch') }}
          </a-button>
        </KfDashboardItem>
      </template>
      <a-table
        class="kf-ant-table"
        :columns="columns"
        :data-source="tableData"
        size="small"
        :pagination="false"
        :scroll="{ y: dashboardBodyHeight - 4 }"
        :empty-text="$t('empty_text')"
        :row-selection="{
          selectedRowKeys: selectedRowKeys,
          onChange: onSelectChange,
          getCheckboxProps: getCheckboxProps,
        }"
        :custom-row="customRow"
      >
        <template
          #bodyCell="{
            column,
            record,
          }: {
            column: AntTableColumn,
            record: KungfuApi.OrderTriggerResolved,
          }"
        >
          <template v-if="column.dataIndex === 'side'">
            <span :class="`color-${dealSide(record.side).color}`">
              {{ dealSide(record.side).name }}
            </span>
          </template>
          <template v-else-if="column.dataIndex === 'offset'">
            <span :class="`color-${dealOffset(record.offset).color}`">
              {{ dealOffset(record.offset).name }}
            </span>
          </template>
          <template v-else-if="column.dataIndex === 'status_uname'">
            <span :class="`color-${record.status_color}`">
              {{ record.status_uname }}
            </span>
          </template>
          <template v-else-if="column.dataIndex === 'dest_uname'">
            <span :class="[`color-${record.dest_resolved_data.color}`]">
              {{ record.dest_uname }}
            </span>
          </template>
          <template v-else-if="column.dataIndex === 'actions'">
            <span
              v-if="orderTriggerCanBeCancel(record)"
              class="color-default"
              @click="handleCancelOrderTrigger(record)"
            >
              {{ t('orderTriggerConfig.trigger_cancel') }}
            </span>
            <LoadingOutlined
              v-if="record.status === OrderTriggerStatusEnum.Cancelling"
            />
          </template>
        </template>
      </a-table>
    </KfDashboard>
    <KfSetByConfigModal
      v-if="batchOrderTriggerVisble"
      v-model:visible="batchOrderTriggerVisble"
      :width="1410"
      :label-col="4"
      :wrapper-col="17"
      :payload="batchOrderTriggerConfigPayload"
      :form-style="{
        maxHeight: '700px',
        overflow: 'auto',
      }"
      @confirm="
        ({ formState }) =>
          handleConfirmBatchOrderTrigger(formState.orderTrigger)
      "
    ></KfSetByConfigModal>
  </div>
</template>
<style lang="less">
.kf-market-data__warp {
  height: 100%;
  .trans-name__txt {
    margin-right: 8px;
  }
}
</style>
