<script setup lang="ts">
import { delayMilliSeconds } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  getIdByKfLocation,
  getProcessIdByKfLocation,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  dealOffset,
  dealSide,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import {
  useActiveInstruments,
  useExtConfigsRelated,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import {
  useDownloadHistoryTradingData,
  useTableSearchKeyword,
  useDashboardBodySize,
  confirmModal,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import {
  DownloadOutlined,
  LoadingOutlined,
  CalendarOutlined,
  PieChartOutlined,
} from '@ant-design/icons-vue';

import {
  computed,
  getCurrentInstance,
  onBeforeUnmount,
  onMounted,
  reactive,
  ref,
  toRaw,
  watch,
  nextTick,
} from 'vue';
import { getColumns } from './config';
import {
  dealOrder,
  getKungfuHistoryData,
  kfCancelAllOrders,
  kfCancelOrder,
  makeOrderByOrderInput,
  getOrderLatencyDataByOrderStat,
  kfCancelAllOrdersTrigger,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import type { Dayjs } from 'dayjs';
import {
  OrderCancelledStatus,
  OrderTriggerCancelStatus,
  UnfinishedOrderStatus,
} from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import {
  HistoryDateEnum,
  OrderStatusEnum,
  OrderActionFlagEnum,
  OrderTriggerStatusEnum,
  OrderTriggerConfigTypeEnum,
  OrderTriggerFlag,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import {
  showTradingDataDetail,
  useCurrentGlobalKfLocation,
  useDealDataWithCaches,
  useProcessStatusDetailData,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import StatisticModal from './OrderStatisticModal.vue';
import { messagePrompt } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { readRootPackageJsonSync } from '@kungfu-trader/kungfu-js-api/utils/fileUtils';

const { t } = VueI18n.global;
const { success, error } = messagePrompt();
const app = getCurrentInstance();
const { getPriceTickAndPrecision } = useActiveInstruments();
const { extConfigs } = useExtConfigsRelated();

const { handleBodySizeChange } = useDashboardBodySize();

const { processStatusData } = useProcessStatusDetailData();
const { dealDataWithCache, clearCaches } = useDealDataWithCaches<
  KungfuApi.Order,
  KungfuApi.OrderResolvedWithoutStat
>(['uid_key', 'update_time']);
const orders = ref<KungfuApi.OrderResolved[]>([]);
const allOrders = ref<KungfuApi.OrderResolved[]>([]);
const { searchKeyword, tableData } =
  useTableSearchKeyword<KungfuApi.OrderResolved>(
    orders,
    [
      'order_id',
      'instrument_id',
      'side',
      'offset',
      'status_uname',
      'exchange_id',
      'source_uname',
      'dest_uname',
    ],
    {
      side: (item) => dealSide(Number(item)).name,
      offset: (item) => dealOffset(Number(item)).name,
    },
  );
const unfinishedOrder = ref<boolean>(false);
const historyDate = ref<Dayjs>();
const historyDataLoading = ref<boolean>();

const {
  currentGlobalKfLocation,
  currentCategoryData,
  getCurrentGlobalKfLocationId,
} = useCurrentGlobalKfLocation(window.watcher);

const { handleDownload } = useDownloadHistoryTradingData();
const adjustOrderMaskVisible = ref(false);
const statisticModalVisible = ref<boolean>(false);
const orderCurrentOrderTriggers = ref<
  Record<string, KungfuApi.OrderTriggerResolved[]>
>({});

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

onMounted(() => {
  if (app?.proxy) {
    const subscription = app.proxy.$tradingDataSubject.subscribe(
      (watcher: KungfuApi.Watcher) => {
        if (historyDate.value) {
          return;
        }

        if (!currentGlobalKfLocation.value) return;

        if (adjustOrderMaskVisible.value) {
          return;
        }

        const ordersResolved =
          globalThis.HookKeeper.getHooks().dealTradingData.trigger(
            window.watcher,
            currentGlobalKfLocation.value,
            watcher.ledger.Order,
            'order',
          ) as KungfuApi.Order[];

        if (unfinishedOrder.value) {
          const tempAllOrders = ordersResolved.map((item) => {
            const { price_precision } = getPriceTickAndPrecision(
              item.instrument_id,
              item.exchange_id,
            );

            return toRaw({
              ...dealDataWithCache(
                item,
                () => dealOrder(watcher, item, price_precision),
                { price_precision },
              ),
              ...getOrderLatencyDataByOrderStat(
                item,
                watcher.ledger.OrderStat,
                price_precision,
              ), // 分离出OrderMedianResolved，解决缓存依赖值变更，但缓存uid_key和update_time不变导致取值错误
            });
          });
          allOrders.value = tempAllOrders;
          orders.value = toRaw(
            tempAllOrders.filter((item) => !isFinishedOrderStatus(item.status)),
          );
          return;
        }

        let finishedOrdersCount = 0;
        const { totalOrders, ordersForTable } = ordersResolved.reduce(
          (preOrders, curOrder) => {
            const { price_precision } = getPriceTickAndPrecision(
              curOrder.instrument_id,
              curOrder.exchange_id,
            );

            const orderResolved = toRaw({
              ...dealDataWithCache(
                curOrder,
                () => dealOrder(watcher, curOrder, price_precision),
                { price_precision },
              ),
              ...getOrderLatencyDataByOrderStat(
                curOrder,
                watcher.ledger.OrderStat,
                price_precision,
              ),
            });
            preOrders.totalOrders.push(orderResolved);
            if (isFinishedOrderStatus(curOrder.status)) {
              if (finishedOrdersCount < 500) {
                finishedOrdersCount++;
                preOrders.ordersForTable.push(orderResolved);
              }
            } else {
              preOrders.ordersForTable.push(orderResolved);
            }
            return preOrders;
          },
          { totalOrders: [], ordersForTable: [] } as {
            totalOrders: KungfuApi.OrderResolved[];
            ordersForTable: KungfuApi.OrderResolved[];
          },
        );

        allOrders.value = toRaw(totalOrders);
        orders.value = toRaw(ordersForTable);

        const source = watcher.getLocationUID(currentGlobalKfLocation.value);
        orderCurrentOrderTriggers.value = watcher.ledger.OrderTrigger.filter(
          'action_flag',
          OrderTriggerFlag.TriggerCancel,
        )
          .filter('source', source)
          .list()
          .reduce((pre, cur) => {
            const order_id = cur.order_id.toString();
            if (order_id in pre) {
              pre[order_id].push(cur);
            } else {
              pre[order_id] = [cur];
            }
            return pre;
          }, {});
      },
    );

    onBeforeUnmount(() => {
      subscription.unsubscribe();
    });
  }
});

watch(currentGlobalKfLocation, () => {
  historyDate.value = undefined;
  allOrders.value = [];
  orders.value = [];
  clearCaches();
});

watch(historyDate, async (newDate) => {
  clearCaches();
  if (!newDate) {
    return;
  }

  if (!currentGlobalKfLocation.value) return;

  orders.value = [];
  allOrders.value = [];
  historyDataLoading.value = true;
  delayMilliSeconds(500)
    .then(() =>
      getKungfuHistoryData(
        window.watcher,
        newDate.format(),
        HistoryDateEnum.naturalDate,
        'Order',
        currentGlobalKfLocation.value as KungfuApi.KfLocation,
      ),
    )
    .then((historyData) => {
      if (!historyData) return;

      const { tradingData } = historyData;

      const orderResolved =
        globalThis.HookKeeper.getHooks().dealTradingData.trigger(
          window.watcher,
          currentGlobalKfLocation.value,
          tradingData.Order,
          'order',
        ) as KungfuApi.Order[];

      const tempAllOrders = toRaw(
        orderResolved.map((item) => {
          const { price_precision } = getPriceTickAndPrecision(
            item.instrument_id,
            item.exchange_id,
          );

          return toRaw({
            ...dealDataWithCache(
              item,
              () => dealOrder(window.watcher, item, price_precision),
              { price_precision },
            ),
            ...getOrderLatencyDataByOrderStat(
              item,
              tradingData.OrderStat,
              price_precision,
            ),
          });
        }),
      );
      allOrders.value = tempAllOrders;
      orders.value = tempAllOrders;
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

function isFinishedOrderStatus(orderStatus: OrderStatusEnum): boolean {
  return !UnfinishedOrderStatus.includes(orderStatus);
}

const cancelOrderTriggerBtnVisible = computed(() => {
  const rootPackageJson = readRootPackageJsonSync();
  if (rootPackageJson?.appConfig?.orderTrigger === false) {
    return false;
  }

  const tdName = currentGlobalKfLocation.value?.group as string;
  const extConfig = extConfigs.value.td[tdName];
  if (
    extConfig &&
    extConfig.orderTrigger[OrderTriggerConfigTypeEnum.CancelOrder]
  ) {
    return true;
  } else {
    return false;
  }
});

function handleCancelOrder(order: KungfuApi.OrderResolved): void {
  if (!currentGlobalKfLocation.value || !window.watcher) {
    error();
    return;
  }

  if (!testOrderSourceIsOnline(order)) {
    return;
  }

  kfCancelOrder(window.watcher, order, OrderActionFlagEnum.Cancel)
    .then(() => {
      success();
    })
    .catch(() => {
      error();
    });
}

function handleCancelAllOrders(): void {
  if (!currentGlobalKfLocation.value || !window.watcher) {
    error();
    return;
  }

  const name = getIdByKfLocation(currentGlobalKfLocation.value);

  confirmModal(
    t('orderConfig.confirm_cancel_all'),
    `${t('orderConfig.confirm')} ${currentCategoryData.value?.name} ${name} ${t(
      'orderConfig.cancel_all',
    )}`,
  ).then((flag) => {
    if (!flag || !currentGlobalKfLocation.value || !window.watcher) {
      return;
    }

    const orders = getTargetCancelOrders();
    return kfCancelAllOrders(window.watcher, orders)
      .then(() => {
        success();
      })
      .catch((err) => {
        error(err.message);
      });
  });
}

function handleInsertOrderTrigger(order: KungfuApi.OrderResolved): void {
  const cancelOrderTrigger = isOrderTriggerHasSubmitted(order.order_id);
  if (cancelOrderTrigger) {
    if (!currentGlobalKfLocation.value || !window.watcher) {
      error();
      return;
    }

    if (isFinishedOrderStatus(order.status)) {
      error(t('orderConfig.order_finished'));
      return;
    }

    kfCancelOrder(window.watcher, order, OrderActionFlagEnum.TriggerCancel)
      .then(() => {
        success();
      })
      .catch((err: Error) => {
        error(err.message);
      });
  } else {
    confirmModal(
      t('orderConfig.confirm_cancel_order_trigger'),
      t('orderConfig.cancel_order_trigger_context'),
    ).then((res) => {
      if (!res || !currentGlobalKfLocation.value || !window.watcher) {
        return;
      }
      const { order_id } = order;
      // 再获取一次, 预防点击弹窗很久不操作, 原订单状态已改变
      const { status } = (
        window.watcher as KungfuApi.Watcher
      ).ledger.Order.filter('order_id', order_id).list()[0];
      if (OrderCancelledStatus.includes(status)) {
        error(t('orderConfig.order_finished'));
        return;
      }
      const orderTriggers =
        orderCurrentOrderTriggers.value[order_id.toString()];
      const submittedOrderTrigger = orderTriggers.filter((orderTrigger) =>
        OrderTriggerCancelStatus.includes(orderTrigger.status),
      );

      return kfCancelAllOrdersTrigger(
        window.watcher,
        submittedOrderTrigger,
        currentGlobalKfLocation.value,
      )
        .then(() => {
          success();
        })
        .catch((err) => {
          error(err.message);
        });
    });
  }
}

function isOrderTriggerHasSubmitted(orderId: bigint) {
  const orderTriggers = orderCurrentOrderTriggers.value[orderId.toString()];
  if (!orderTriggers || orderTriggers.length === 0) return true;
  const submittedOrderTrigger = orderTriggers.filter(
    (orderTrigger) => orderTrigger.status === OrderTriggerStatusEnum.Submitted,
  );
  if (submittedOrderTrigger.length > 0) return false;
  return true;
}

function isOrderTriggerHasPending(orderId: bigint) {
  const orderTriggers = orderCurrentOrderTriggers.value[orderId.toString()];
  if (!orderTriggers || orderTriggers.length === 0) return false;
  const submittedOrderTrigger = orderTriggers.filter(
    (orderTrigger) => orderTrigger.status === OrderTriggerStatusEnum.Pending,
  );
  if (submittedOrderTrigger.length > 0) return true;
  return false;
}

function filterUnfinishedOrders(orders: KungfuApi.Order[]): KungfuApi.Order[] {
  return orders.filter((item) => UnfinishedOrderStatus.includes(item.status));
}

function getTargetCancelOrders(): KungfuApi.Order[] {
  if (!currentGlobalKfLocation.value || !window.watcher) {
    return [];
  }

  return filterUnfinishedOrders(
    globalThis.HookKeeper.getHooks().dealTradingData.trigger(
      window.watcher,
      currentGlobalKfLocation.value,
      window.watcher.ledger.Order,
      'order',
    ) as KungfuApi.Order[],
  );
}

function handleShowTradingDataDetail({
  row,
}: {
  event: MouseEvent;
  row: KungfuApi.TradingDataItem;
}) {
  showTradingDataDetail(row, t('orderConfig.entrust'));
}

const adjustOrderConfig = reactive({
  clientWidth: 0,
  clientHeight: 0,
  offsetLeft: 0,
  offsetTop: 0,
});
const adjustOrderForm = ref<{
  price: number;
  volume: number;
}>({
  price: 0,
  volume: 0,
});
const adjustOrder = ref<KungfuApi.OrderResolved | null>(null);
const tableRef = ref();
const adjustNumberInputRef = ref();
const adjustPriceTick = ref<number>();

function handleAdjustOrder(data: {
  event: MouseEvent;
  row: KungfuApi.TradingDataItem;
  column: KfTradingDataTableHeaderConfig;
}): void {
  const { event, row, column } = data;
  const order = row as KungfuApi.OrderResolved;
  let target = event.target as HTMLElement | null;

  if (column.dataIndex !== 'limit_price_resolved') {
    return;
    // if (column.dataIndex !== 'volume_left') {
    //   return;
    // }
  }

  if (!currentGlobalKfLocation.value || !window.watcher) {
    return;
  }

  if (
    !testOrderSourceIsOnline(order) ||
    order.status === OrderStatusEnum.Cancelling
  ) {
    return;
  }

  if (target) {
    if (target.tagName !== 'LI') {
      target = target.parentNode as HTMLElement;
    }
    adjustOrderMaskVisible.value = true;
    const rectData = target.getBoundingClientRect();
    const tableRectData = tableRef.value.getBoundingClientRect();
    const deltaTop = rectData.top - tableRectData.top;
    adjustOrderConfig.clientWidth = target.clientWidth;
    adjustOrderConfig.clientHeight = target.clientHeight;
    adjustOrderConfig.offsetTop = deltaTop; //header height

    if (column.dataIndex === 'limit_price_resolved') {
      adjustOrderConfig.offsetLeft = target.offsetLeft;
    } else {
      adjustOrderConfig.offsetLeft = target.offsetLeft - target.clientWidth;
    }

    adjustOrderForm.value.price = order.limit_price;
    // adjustOrderForm.value.volume = +Number(order.volume_left);
    adjustOrder.value = order;

    const { price_tick } = getPriceTickAndPrecision(
      order.instrument_id,
      order.exchange_id,
    );

    adjustPriceTick.value = price_tick;

    nextTick().then(() => {
      if (!adjustNumberInputRef.value) return;
      adjustNumberInputRef.value.focus();
    });
  }
}

function handleClickAdjustOrderMask(): void {
  if (!adjustOrderMaskVisible.value) {
    return;
  }
  const kfLocation = currentGlobalKfLocation.value;
  if (!kfLocation) {
    error(t('location_error'));
    adjustOrderMaskVisible.value = false;
    return;
  }

  const order = adjustOrder.value;
  if (!order) {
    adjustOrderMaskVisible.value = false;
    return;
  }

  if (!testOrderSourceIsOnline(order)) {
    adjustOrderMaskVisible.value = false;
    error(
      t('tradingConfig.finished_msg', {
        status: order.status,
      }),
    );
    return;
  }

  if (+adjustOrderForm.value.price <= 0) {
    return;
  }

  if (+order.limit_price === +adjustOrderForm.value.price) {
    adjustOrderMaskVisible.value = false;
    return;
  }

  adjustOrderMaskVisible.value = false;
  const { source } = order;
  const sourceLocation = window.watcher.getLocation(source);
  kfCancelOrder(window.watcher, order, OrderActionFlagEnum.Cancel)
    .then(() => {
      const makeOrderInput: KungfuApi.MakeOrderInput = {
        instrument_id: order.instrument_id,
        instrument_type: order.instrument_type,
        exchange_id: order.exchange_id,
        limit_price: +adjustOrderForm.value.price,
        volume: Number(order.volume_left),
        price_type: +order.price_type,
        side: +order.side,
        offset: +order.offset,
        hedge_flag: +order.hedge_flag,
        is_swap: !!order.is_swap,
        parent_id: 0n,
        contract_id: '',
      };

      return makeOrderByOrderInput(
        window.watcher,
        makeOrderInput,
        sourceLocation,
        getIdByKfLocation(window.watcher.getLocation(order.source)),
      );
    })
    .then(() => {
      success();
    })
    .catch((err) => {
      error(err.message);
    })
    .finally(() => {
      adjustOrderMaskVisible.value = false;
    });
}

function handleCloseAdjustOrderMask() {
  adjustOrderMaskVisible.value = false;
}

function testOrderSourceIsOnline(order: KungfuApi.OrderResolved) {
  if (!window.watcher) {
    return false;
  }

  const { source, status } = order;
  const tdLocation = window.watcher.getLocation(source);
  const processId = getProcessIdByKfLocation(tdLocation);
  if (processStatusData.value[processId] !== 'online') {
    error(`${t('orderConfig.start')} ${processId} ${t('orderConfig.td')}`);
    return false;
  }

  if (isFinishedOrderStatus(status)) {
    return false;
  }

  return true;
}
</script>
<template>
  <div class="kf-orders__warp kf-translateZ">
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
          <a-checkbox v-model:checked="unfinishedOrder" size="small">
            {{ $t('orderConfig.show_pending_orders') }}
          </a-checkbox>
        </KfDashboardItem>
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
            :placeholder="$t('orderConfig.date_picker')"
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
            @click="handleDownload('Order', currentGlobalKfLocation)"
          >
            <template #icon>
              <DownloadOutlined style="font-size: 14px" />
            </template>
          </a-button>
        </KfDashboardItem>
        <KfDashboardItem>
          <a-button
            size="small"
            type="primary"
            danger
            @click="handleCancelAllOrders"
          >
            {{ $t('orderConfig.cancel_all') }}
          </a-button>
        </KfDashboardItem>
      </template>
      <div ref="tableRef" class="kf-table__warp">
        <div v-if="adjustOrderMaskVisible" class="kf-adjust-order-mask__warp">
          <div
            class="kf-adjust-order-mask"
            @click.stop.once="handleCloseAdjustOrderMask"
          ></div>
          <div
            class="adjust-order-wrap"
            :style="{
              top: adjustOrderConfig.offsetTop + 'px',
              left: adjustOrderConfig.offsetLeft + 'px',
            }"
          >
            <a-input-number
              v-if="adjustOrderConfig.clientWidth !== 0"
              ref="adjustNumberInputRef"
              v-model:value="adjustOrderForm.price"
              string-mode
              :step="adjustPriceTick"
              class="adjust-order-item price"
              :style="{
                width: adjustOrderConfig.clientWidth + 'px',
                height: adjustOrderConfig.clientHeight + 'px',
              }"
              @keyup.esc="handleCloseAdjustOrderMask"
              @blur="handleCloseAdjustOrderMask"
              @keyup.enter="handleClickAdjustOrderMask"
            ></a-input-number>
          </div>
        </div>
        <KfTradingDataTable
          :columns="columns"
          :data-source="tableData"
          key-field="uid_key"
          @clickCell="handleAdjustOrder"
          @rightClickRow="handleShowTradingDataDetail"
        >
          <template
            #default="{
              item,
              column,
            }: {
              item: KungfuApi.OrderResolved,
              column: KfTradingDataTableHeaderConfig,
            }"
          >
            <template v-if="column.dataIndex === 'side'">
              <span :class="`color-${dealSide(item.side).color}`">
                {{ dealSide(item.side).name }}
              </span>
            </template>
            <template v-else-if="column.dataIndex === 'offset'">
              <span :class="`color-${dealOffset(item.offset).color}`">
                {{ dealOffset(item.offset).name }}
              </span>
            </template>
            <template v-else-if="column.dataIndex === 'volume_left'">
              <span
                style="float: right"
                :title="`${item.volume - item.volume_left} / ${item.volume}`"
              >
                {{ `${item.volume - item.volume_left} / ${item.volume}` }}
              </span>
            </template>
            <template v-else-if="column.dataIndex === 'status_uname'">
              <span :class="`color-${item.status_color}`">
                {{ item.status_uname }}
              </span>
            </template>
            <template v-else-if="column.dataIndex === 'source_uname'">
              <span :class="[`color-${item.source_resolved_data.color}`]">
                {{ item.source_uname }}
              </span>
            </template>
            <template v-else-if="column.dataIndex === 'dest_uname'">
              <span :class="[`color-${item.dest_resolved_data.color}`]">
                {{ item.dest_uname }}
              </span>
            </template>
            <template v-else-if="column.dataIndex === 'actions'">
              <span
                v-if="!isFinishedOrderStatus(item.status)"
                class="color-red"
                style="margin-right: 8px; margin-left: 2px"
                @click="handleCancelOrder(item)"
              >
                {{ $t('orderConfig.cancel_order') }}
              </span>
              <span
                v-if="
                  !isFinishedOrderStatus(item.status) &&
                  cancelOrderTriggerBtnVisible
                "
                :class="{
                  'color-default': isOrderTriggerHasSubmitted(item.order_id),
                  'color-yellow': !isOrderTriggerHasSubmitted(item.order_id),
                }"
                @click="handleInsertOrderTrigger(item)"
              >
                {{ $t('orderConfig.cancel_order_trigger') }}
              </span>
              <LoadingOutlined v-if="isOrderTriggerHasPending(item.order_id)" />
            </template>
          </template>
        </KfTradingDataTable>
      </div>
    </KfDashboard>
    <StatisticModal
      v-if="statisticModalVisible"
      v-model:visible="statisticModalVisible"
      :orders="allOrders"
      :is-unfinished-order="unfinishedOrder"
      :history-date="historyDate"
    ></StatisticModal>
  </div>
</template>
<style lang="less">
.kf-orders__warp {
  width: 100%;
  height: 100%;

  .kf-table__warp {
    width: 100%;
    height: 100%;
    position: relative;

    .kf-trading-data-table {
      width: 100%;
      height: 100%;
    }

    .kf-adjust-order-mask__warp {
      position: absolute;
      width: 100%;
      height: 100%;
      left: 0;
      top: 0;
      z-index: 100;

      .kf-adjust-order-mask {
        position: absolute;
        width: 100%;
        height: 100%;
        left: 0;
        top: 0;
        z-index: 100;
        background: rgba(0, 0, 0, 0.45);
      }

      .adjust-order-wrap {
        position: absolute;
        z-index: 101;
      }
      .adjust-order-item {
        background: #141414;

        .ant-input-number-input-wrap {
          height: 100%;

          .ant-input-number-input {
            height: 100%;
          }
        }
      }
    }
  }
}
</style>
