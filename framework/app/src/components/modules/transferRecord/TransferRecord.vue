<script setup lang="ts">
import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';

import {
  useDashboardBodySize,
  useTableSearchKeyword,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { getColumns } from './config';
import {
  getCurrentInstance,
  onBeforeUnmount,
  onActivated,
  onDeactivated,
  ref,
} from 'vue';

import {
  useCoreBindPage,
  useCurrentGlobalKfLocation,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { dealKfNumber } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { ArrowRightOutlined } from '@ant-design/icons-vue';
import { FundTransEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { FundTransType } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import { dealKungfuColorToClassname } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';

useCoreBindPage();

const { t } = VueI18n.global;

const app = getCurrentInstance();
const { dashboardBodyHeight, handleBodySizeChange } = useDashboardBodySize();
const { currentGlobalKfLocation } = useCurrentGlobalKfLocation(window.watcher);
const columns = getColumns();

let tableList = ref<KungfuApi.TimeKeyValue[]>([]);
let tableListResolved = ref<KungfuApi.TransferRecordResolved[]>([]);
const { searchKeyword, tableData } =
  useTableSearchKeyword<KungfuApi.TransferRecordResolved>(tableListResolved, [
    'update_time',
    'source',
    'target',
    'amount',
    'trans_type_resolved',
    'message',
  ]);

onActivated(() => {
  if (app?.proxy && 0) {
    const subscription = app.proxy.$tradingDataSubject.subscribe((data) => {
      const { watcher } = data;
      if (!currentGlobalKfLocation.value) return;
      const source = watcher.getLocationUID(currentGlobalKfLocation.value);
      tableList.value = (
        watcher.ledger[
          'TimeKeyValue'
        ] as KungfuApi.DataTable<KungfuApi.TimeKeyValue>
      )
        .filter('tag_a', 'FundTrans')
        .filter('source', source)
        .list();
      tableListResolved.value = tableList.value.map(
        (item: KungfuApi.TimeKeyValue) => {
          const value = JSON.parse(item.value);
          let message: string, status: FundTransEnum;
          if (!value.ret) {
            message = t('fundTrans.pending');
            status = FundTransEnum.Pending;
          } else if (value.ret && value.ret < 0) {
            message = value.message || t('fundTrans.error');
            status = FundTransEnum.Error;
          } else {
            message = t('fundTrans.success');
            status = FundTransEnum.Success;
          }
          const result: KungfuApi.TransferRecordResolved = {
            update_time: BigInt(value.update_time || 0n),
            source: value.source || '--',
            target: value.target || '--',
            amount: value.amount || 0,
            trans_type: value.key,
            status,
            message,
            trans_type_resolved: getTransTypeResolved(value.key),
          };
          return result;
        },
      );
    });

    onBeforeUnmount(() => {
      subscription.unsubscribe();
    });

    onDeactivated(() => {
      subscription.unsubscribe();
    });
  }
});

function getTransTypeResolved(
  type: 'FundTransIn' | 'FundTransBetweenNodes' | 'FundTransOut',
): string {
  if (type === 'FundTransBetweenNodes') {
    return 'HTS->HTS';
  } else if (type === 'FundTransIn') {
    return `${t('fundTrans.centralized_counter')}->HTS`;
  } else if (type === 'FundTransOut') {
    return `HTS->${t('fundTrans.centralized_counter')}`;
  } else {
    return '';
  }
}
</script>
<template>
  <div class="kf-market-data__warp">
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
      <a-table
        class="kf-ant-table"
        :columns="columns"
        :data-source="tableData"
        size="small"
        :pagination="false"
        :scroll="{ y: dashboardBodyHeight - 4 }"
        :empty-text="$t('empty_text')"
      >
        <template
          #bodyCell="{
            column,
            record,
          }: {
            column: AntTableColumn,
            record: KungfuApi.TransferRecordResolved,
          }"
        >
          <template v-if="column.dataIndex === 'update_time'">
            <div>{{ dealKfTime(record.update_time, true) }}</div>
          </template>
          <template v-if="column.dataIndex === 'status'">
            <span
              :title="record.message"
              :class="
                dealKungfuColorToClassname(FundTransType[record.status].color)
              "
            >
              {{ record.message }}
            </span>
          </template>
          <template v-if="column.dataIndex === 'trans_type'">
            <div v-if="record.trans_type === 'FundTransBetweenNodes'">
              <span class="trans-name__txt">HTS</span>
              <ArrowRightOutlined style="margin-right: 8px; font-size: 10px" />
              <span class="trans-name__txt">HTS</span>
            </div>
            <div v-if="record.trans_type === 'FundTransIn'">
              <span class="trans-name__txt">
                {{ t('fundTrans.centralized_counter') }}
              </span>
              <ArrowRightOutlined style="margin-right: 8px; font-size: 10px" />
              <span class="trans-name__txt">HTS</span>
            </div>
            <div v-if="record.trans_type === 'FundTransOut'">
              <span class="trans-name__txt">HTS</span>
              <ArrowRightOutlined style="margin-right: 8px; font-size: 10px" />
              <span class="trans-name__txt">
                {{ t('fundTrans.centralized_counter') }}
              </span>
            </div>
          </template>
          <template v-if="column.dataIndex === 'amount'">
            <div>{{ dealKfNumber(record.amount) }}</div>
          </template>
        </template>
      </a-table>
    </KfDashboard>
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
