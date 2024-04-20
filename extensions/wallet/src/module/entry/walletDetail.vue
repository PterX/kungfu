<template>
  <a-drawer
    ref="walletDetail"
    v-model:visible="visible"
    class="wallet-detail"
    :title="$t('awsWallet.costRecords')"
    :closable="false"
    height="100%"
    :get-container="getContainer"
    placement="bottom"
    :style="{ position: 'absolute' }"
  >
    <template #extra>
      <DownOutlined style="transform: scaleX(1.3)" @click="visible = false" />
    </template>
    <AuthAccessMask
      :access="['backtest_whitelisted']"
      :name="$t('backtest.backtest')"
    />
    <div class="detail-content">
      <div class="detail-content-title">
        <a-statistic
          :title="$t('awsWallet.balance')"
          :value="dealKfNumber(walletStore.currentBalance, 2)"
          :suffix="$t('awsWallet.unit')"
        />
        <a-button
          style="margin-right: 8px"
          @click="handleRefreshBalance(false)"
        >
          <template #icon><SyncOutlined /></template>
        </a-button>
        <a-button type="primary" @click="handleToRecharge">
          {{ t('awsWallet.recharge') }}
        </a-button>
      </div>
      <div class="detail-content-body">
        <div class="detail-content-body-chart">
          <div class="detail-content-body-chart">
            <div class="detail-content-body-chart-title">
              <div class="detail-content-body-chart-title-text">
                {{ t('awsWallet.costRecords') }}
              </div>
              <a-range-picker
                v-model:value="dateRange"
                :disabled-date="isDisableTime"
                :locale="locale"
                :allow-clear="false"
              ></a-range-picker>
            </div>
            <div class="detail-content-body-chart-content">
              <KfTradingCharts :option="chartOptions"></KfTradingCharts>
            </div>
          </div>
        </div>
        <div class="detail-content-body-table">
          <div class="detail-content-body-table-title">
            <div class="detail-content-body-table-title-text">
              {{ t('awsWallet.transRecords') }}
            </div>
            <a-range-picker
              v-model:value="dateRange"
              :disabled-date="isDisableTime"
              :locale="locale"
              :allow-clear="false"
            ></a-range-picker>
          </div>
          <a-table
            class="kf-ant-table"
            :columns="TransRecordsTableColumns"
            :data-source="allTransactions"
            size="small"
            pagination
            :scroll="{ y: '100%' }"
            :empty-text="$t('empty_text')"
          >
            <template
              #bodyCell="{
                column,
                record,
              }: {
                column: AntTableColumn,
                record: WalletTransData,
              }"
            >
              <template v-if="column.dataIndex === 'time'">
                {{ dayjs(record.settled_at).format('YYYY-MM-DD HH:mm:ss') }}
              </template>
              <template v-else-if="column.dataIndex === 'transaction_type'">
                <span
                  :class="
                    dealKungfuColorToClassname(
                      transTypeConfig[record.transaction_type].color ||
                        'default',
                    )
                  "
                >
                  {{ transTypeConfig[record.transaction_type].name }}
                </span>
              </template>
              <template v-else-if="column.dataIndex === 'note'">--</template>
              <template v-else-if="column.dataIndex === 'amount'">
                {{ (record.amount || 0) > 0 ? '+' : '' }}{{ record.amount }}
              </template>
            </template>
          </a-table>
        </div>
      </div>
    </div>
  </a-drawer>
</template>

<script lang="ts" setup>
import { ref, getCurrentInstance, computed, watch } from 'vue';
import { DownOutlined, SyncOutlined } from '@ant-design/icons-vue';
import dayjs, { Dayjs } from 'dayjs';
import DayjsZhCN from 'dayjs/locale/zh-cn';
import weekday from 'dayjs/plugin/weekday';
import localeData from 'dayjs/plugin/localeData';
import * as echarts from 'echarts';

import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import {
  dealKfNumber,
  dealKfDecimalPrecision,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  dealKungfuColorToClassname,
  messagePrompt,
  onClickOutside,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import KfTradingCharts from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingCharts.vue';
import AuthAccessMask from '@kungfu-trader/kfx-ui-login-authing/src/components/AuthAccessMask.vue';

import { useWalletStore } from '../../store';
import {
  AwsWalletKeys,
  TransRecordsTableColumns,
  transTypeConfig,
} from '../../configs';
import { WalletTransData } from '../../typings';

dayjs.extend(weekday);
dayjs.extend(localeData);

const props = defineProps<{
  toggle: boolean;
}>();

const emit = defineEmits<{
  (e: 'update:toggle', val: boolean): void;
}>();

const { t } = VueI18n.global;
const locale = { ...DayjsZhCN, shortMonths: DayjsZhCN.monthsShort };
const app = getCurrentInstance();
const message = messagePrompt();
const walletStore = useWalletStore();

const visible = ref(props.toggle);
const walletDetail = ref();
const dateRange = ref<dayjs.Dayjs[]>([]);

let deregister: (() => void) | null = null;

const getDefaultDateRange = () => {
  const byDay = walletStore.currentWallet?.transactions.byDay;
  if (!byDay || Object.keys(byDay).length === 0) {
    return [];
  }

  const days = Object.keys(byDay).sort((a, b) => +a - +b);

  const startTs = +days[0];
  const endTs = +days[days.length - 1];
  const today = dayjs();
  const todayTs = today.valueOf();
  const endResolved = Math.max(endTs, todayTs);

  const delta = Math.abs(dayjs(startTs).diff(dayjs(endResolved), 'day'));
  const deltaResolved = Math.min(delta, 30);

  return [dayjs().subtract(deltaResolved, 'day').startOf('day'), today];
};

watch(
  () => walletStore.currentWallet?.transactions.byDay,
  (newVal) => {
    if (!newVal || dateRange.value.length !== 0) return;
    dateRange.value = getDefaultDateRange();
  },
  {
    deep: true,
  },
);

const allTransactions = computed(() => {
  if (!walletStore.currentWallet || dateRange.value.length === 0) return [];

  const start = dateRange.value[0].startOf('day').valueOf();
  const end = dateRange.value[1].endOf('day').valueOf();
  return walletStore.currentWallet.transactions.all.filter((trans) => {
    if (trans.created_at) {
      return trans.created_at >= start && trans.created_at <= end;
    }
    return false;
  });
});

const allTransByDayForChart = computed(() => {
  if (!walletStore.currentWallet || dateRange.value.length === 0) return {};

  const delta = Math.abs(dateRange.value[0].diff(dateRange.value[1], 'day'));
  const days = new Array(delta + 1).fill(0).map((_, i) => {
    return dateRange.value[0].add(i, 'day').valueOf();
  });

  const daysAmount = days.map((day) => {
    if (walletStore.currentWallet) {
      const amount =
        walletStore.currentWallet.transactions.byDay[day]?.outbound ?? 0;
      return dealKfDecimalPrecision(amount);
    }

    return 0;
  });

  return {
    days: days.map((day) => dayjs(+day).format('YYYY-MM-DD')),
    daysAmount,
  };
});

const chartOptions = computed<echarts.EChartsCoreOption>(() => {
  const options: echarts.EChartsCoreOption = {
    grid: {
      left: '8',
      right: '8',
      bottom: '16',
      top: '20',
      containLabel: true,
    },
    tooltip: {
      show: true,
      confine: true,
      trigger: 'axis',
      axisPointer: { type: 'shadow' },
    },
    xAxis: {
      type: 'category',
      data: allTransByDayForChart.value.days || [],
    },
    yAxis: {
      type: 'value',
      splitLine: {
        lineStyle: {
          color: 'rgba(255, 255, 255, 0.1)',
        },
      },
    },
    series: [
      {
        data: allTransByDayForChart.value.daysAmount || [],
        type: 'line',
      },
    ],
  };

  return options;
});

watch(
  () => props.toggle,
  (newVal) => {
    visible.value = newVal;
  },
);

watch(
  () => visible.value,
  (newVal) => {
    emit('update:toggle', newVal);
    if (newVal) {
      deregister = onClickOutside('.wallet-detail', () => {
        visible.value = false;
        deregister?.();
      });
    } else {
      deregister?.();
    }
  },
);

const getContainer = () => {
  const el = document.querySelector(
    '.kf-layout > section > section > .ant-layout-content',
  );

  if (el) {
    (el as HTMLElement).style.position = 'relative';
    (el as HTMLElement).classList.add('fixed-width');
  }
  return el;
};

const handleRefreshBalance = (silent = false) => {
  if (!walletStore.currentWallet) return Promise.reject();

  return walletStore
    .refreshAll()
    .then(() => {
      dateRange.value = getDefaultDateRange();
      !silent && message.success();
    })
    .catch((err) => {
      !silent &&
        message.error(
          `${t('awsWallet.getError')}${
            err?.message ? ': ' + err?.message : ''
          }`,
        );
    });
};

const handleToRecharge = () => {
  app?.proxy?.$globalBus.next({
    tag: AwsWalletKeys.ToRecharge,
  });
};

const isDisableTime = (date: Dayjs) => {
  if (!walletStore.currentWallet) return true;
  const days = Object.keys(walletStore.currentWallet.transactions.byDay).sort(
    (a, b) => +a - +b,
  );
  const dateTs = date.valueOf();
  const startTs = +days[0];
  const endTs = +days[days.length - 1];
  const todayTs = dayjs().valueOf();
  const endResolved = Math.max(endTs, todayTs);
  return dateTs < startTs || dateTs > endResolved;
};
</script>

<style lang="less">
.fixed-width {
  width: 100% !important;
}

.detail-content {
  padding: 12px 40px 0;

  .detail-content-title {
    display: flex;
    align-items: flex-end;

    .ant-statistic {
      margin-right: 16px;

      .ant-statistic-title {
        color: rgba(255, 255, 255, 0.85);
        font-size: 20px;
      }

      .ant-statistic-content {
        font-size: 24px;
        color: #faad14;
      }
    }
  }

  .detail-content-body-table {
    margin-top: 44px;

    .detail-content-body-table-title {
      font-size: 16px;
      margin-bottom: 16px;
      display: flex;

      .detail-content-body-table-title-text {
        margin-right: 32px;
      }
    }
  }

  .detail-content-body-chart {
    margin-top: 28px;

    .detail-content-body-chart-title {
      font-size: 16px;
      margin-bottom: 12px;
      display: flex;

      .detail-content-body-chart-title-text {
        margin-right: 32px;
      }
    }

    .detail-content-body-chart-content {
      height: 300px;
    }
  }

  .detail-content-body-table {
    .ant-table-thead {
      tr {
        th {
          background-color: #2d2d2d !important;
        }
      }
    }
  }
}
</style>
