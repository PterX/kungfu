<script setup lang="ts">
import { ref, computed, toRefs, ComputedRef } from 'vue';
import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import KfSetByConfigModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfSetByConfigModal.vue';
import KfReplaySettingModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfReplaySettingModal.vue';
import Icon, {
  FileTextOutlined,
  SettingOutlined,
  DeleteOutlined,
  FormOutlined,
  EyeOutlined,
  HistoryOutlined,
} from '@ant-design/icons-vue';

import {
  useTableSearchKeyword,
  useDashboardBodySize,
  handleOpenLogview,
  handleOpenJournalView,
  handleOpenCodeView,
  messagePrompt,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  handleSwitchProcessStatusGenerator,
  useAddUpdateRemoveKfConfig,
  useAllKfConfigData,
  useAssets,
  useCurrentGlobalKfLocation,
  useProcessStatusDetailData,
  useSwitchAllConfig,
  useReplay,
  useExtConfigsRelated,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';

import {
  getIfProcessRunning,
  getIfProcessStopping,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import {
  dealKfNumber,
  getProcessIdByKfLocation,
  getConfigValue,
  buildTableColumnSorterWithStrike,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { getColumns, setStrategyConfig } from './config';
import path from 'path';
import KfBlinkNum from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfBlinkNum.vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';
import { storeToRefs } from 'pinia';

const { t } = VueI18n.global;
const { success, error } = messagePrompt();

const handleSwitchProcessStatus = handleSwitchProcessStatusGenerator();
const { dashboardBodyHeight, handleBodySizeChange } = useDashboardBodySize();

const setStrategyModalVisible = ref<boolean>(false);

const setStrategyConfigPayload = ref<KungfuApi.SetKfConfigPayload>({
  type: 'add',
  title: t('strategyConfig.strategy'),
  config: {} as KungfuApi.KfStrategyExtConfig,
});

const { strategy } = toRefs(useAllKfConfigData());
const strategyResolved = computed(() => {
  return strategy.value.filter((item) => item.group === 'default');
});
const strategyIdList = computed(() => {
  return strategyResolved.value.map(
    (item: KungfuApi.KfLocation): string => item.name,
  );
});

const { dealRowClassName, customRow } = useCurrentGlobalKfLocation(
  window.watcher,
);

const { processStatusData } = useProcessStatusDetailData();
const { allProcessOnline, handleSwitchAllProcessStatus } = useSwitchAllConfig(
  strategyResolved,
  processStatusData,
);

const {
  replayConfig,
  setReplayModalVisible,
  sessionOptions,
  handleOpenReplayConfirmView,
  handleReplayModal,
} = useReplay();
const { searchKeyword, tableData } = useTableSearchKeyword<KungfuApi.KfConfig>(
  strategyResolved as ComputedRef<KungfuApi.KfConfig[]>,
  ['name'],
);

const { uiExtConfigs } = useExtConfigsRelated();

const StrategyHeaderRightComponentConfigs = computed(() => {
  return Object.keys(uiExtConfigs.value)
    .filter(
      (key) => uiExtConfigs.value[key].position === 'strategy_header_right',
    )
    .map((key) => {
      return {
        ...uiExtConfigs.value[key],
        key,
      };
    });
});

const tableDataResolved = computed(() => {
  return [...tableData.value].sort((a, b) => {
    const aAddTime = getConfigValue(a).add_time || 0;
    const bAddTime = getConfigValue(b).add_time || 0;
    return bAddTime - aAddTime;
  });
});
const { getAssetsByKfConfig } = useAssets();

const { handleConfirmAddUpdateKfConfig, handleRemoveKfConfig } =
  useAddUpdateRemoveKfConfig();

const columns = getColumns((dataIndex) => {
  return buildTableColumnSorterWithStrike<KungfuApi.KfConfig, KungfuApi.Asset>(
    'num',
    dataIndex,
    (kfConfig: KungfuApi.KfConfig) => {
      const { assets } = storeToRefs(useGlobalStore());
      const processId = getProcessIdByKfLocation(kfConfig);
      return assets.value[processId]
        ? assets.value[processId][dataIndex]
        : '--';
    },
  );
});

const getPrefixByLocation = (kfLocation: KungfuApi.KfLocation) =>
  globalThis.HookKeeper.getHooks().prefix.trigger(kfLocation);

async function handleOpenSetStrategyDialog(
  type: KungfuApi.ModalChangeType,
  strategyConfig?: KungfuApi.KfConfig,
) {
  setStrategyConfigPayload.value.type = type;
  setStrategyConfigPayload.value.config =
    await globalThis.HookKeeper.getHooks().resolveExtConfig.trigger(
      {
        category: 'strategy',
        group: 'default',
        name: '*',
        mode: '*',
      },
      setStrategyConfig,
    );
  setStrategyConfigPayload.value.initValue = undefined;

  if (type === 'update') {
    if (strategyConfig) {
      setStrategyConfigPayload.value.initValue = JSON.parse(
        strategyConfig.value,
      );
    }
  }

  setStrategyModalVisible.value = true;
}

function getStrategyPathShowName(kfConfig: KungfuApi.KfConfig): string {
  const strategyPath = getConfigValue(kfConfig).file_path || '';
  return path.basename(strategyPath);
}

function handleRemoveStrategy(record: KungfuApi.KfConfig) {
  return handleRemoveKfConfig(window.watcher, record, processStatusData.value)
    .then(() => {
      success();
    })
    .catch((err) => {
      error(err.message || t('operation_failed'));
    });
}
function handleOpenCodeViewResolved(record: KungfuApi.KfConfig) {
  const processId = getProcessIdByKfLocation(record);
  const filePath = getConfigValue(record).file_path;
  return handleOpenCodeView(processId, filePath, false);
}
</script>

<template>
  <div class="kf-strategy__warp kf-translateZ">
    <KfDashboard @boardSizeChange="handleBodySizeChange">
      <template v-slot:header>
        <KfDashboardItem>
          <a-input-search
            v-model:value="searchKeyword"
            :placeholder="$t('keyword_input')"
            style="width: 120px"
          />
        </KfDashboardItem>
        <KfDashboardItem>
          <a-switch
            :checked="allProcessOnline"
            @click="handleSwitchAllProcessStatus"
          ></a-switch>
        </KfDashboardItem>
        <KfDashboardItem
          v-for="config in StrategyHeaderRightComponentConfigs"
          :key="config.key"
        >
          <component :is="config.key"></component>
        </KfDashboardItem>
        <KfDashboardItem>
          <a-button
            size="small"
            type="primary"
            @click="handleOpenSetStrategyDialog('add')"
          >
            {{ $t('strategyConfig.add_strategy') }}
          </a-button>
        </KfDashboardItem>
      </template>
      <a-table
        class="kf-ant-table"
        :columns="columns"
        :data-source="tableDataResolved"
        size="small"
        :pagination="false"
        :scroll="{ y: dashboardBodyHeight - 4 }"
        :rowClassName="dealRowClassName"
        :customRow="customRow"
        :defaultExpandAllRows="true"
        :emptyText="$t('empty_text')"
      >
        <template
          #bodyCell="{
            column,
            record,
          }: {
            column: AntTableColumn,
            record: KungfuApi.KfConfig,
          }"
        >
          <template v-if="column.dataIndex === 'name'">
            <span>{{ record[column.dataIndex] }}</span>
            <Icon
              v-if="getPrefixByLocation(record).prefixType === 'icon'"
              :component="getPrefixByLocation(record).prefix"
              style="font-size: 12px; margin-left: 7px"
            />
          </template>
          <template v-else-if="column.dataIndex === 'remarks'">
            {{
              JSON.parse((record as KungfuApi.KfConfig).value).remarks || '--'
            }}
          </template>
          <template v-else-if="column.dataIndex === 'strategyFile'">
            {{ getStrategyPathShowName(record) }}
          </template>
          <template v-else-if="column.dataIndex === 'processStatus'">
            <a-switch
              size="small"
              :checked="
                getIfProcessRunning(
                  processStatusData,
                  getProcessIdByKfLocation(record),
                )
              "
              :loading="
                getIfProcessStopping(
                  processStatusData,
                  getProcessIdByKfLocation(record),
                )
              "
              @click="(checked: boolean, Event: MouseEvent) => handleSwitchProcessStatus(checked, Event, record)"
            ></a-switch>
          </template>
          <template v-else-if="column.dataIndex === 'unrealizedPnl'">
            <KfBlinkNum
              mode="compare-zero"
              :num="dealKfNumber(getAssetsByKfConfig(record).unrealized_pnl, 4)"
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'marketValue'">
            <KfBlinkNum
              :num="dealKfNumber(getAssetsByKfConfig(record).market_value, 4)"
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'actions'">
            <div class="kf-actions__warp">
              <HistoryOutlined
                style="font-size: 12px"
                @click.stop="handleOpenReplayConfirmView(record)"
              ></HistoryOutlined>
              <EyeOutlined
                style="font-size: 14px"
                @click.stop="handleOpenJournalView(record)"
              ></EyeOutlined>
              <FileTextOutlined
                style="font-size: 12px"
                @click.stop="handleOpenLogview(record)"
              />
              <FormOutlined
                style="font-size: 12px"
                @click.stop="handleOpenCodeViewResolved(record)"
              />
              <SettingOutlined
                style="font-size: 12px"
                @click.stop="handleOpenSetStrategyDialog('update', record)"
              />
              <DeleteOutlined
                style="font-size: 12px"
                @click.stop="handleRemoveStrategy(record)"
              />
            </div>
          </template>
        </template>
      </a-table>
    </KfDashboard>
    <KfSetByConfigModal
      v-if="setStrategyModalVisible"
      :width="420"
      v-model:visible="setStrategyModalVisible"
      :payload="setStrategyConfigPayload"
      :primaryKeyAvoidRepeatCompareTarget="strategyIdList"
      @confirm="handleConfirmAddUpdateKfConfig($event, 'strategy', 'default')"
    ></KfSetByConfigModal>
    <KfReplaySettingModal
      v-if="setReplayModalVisible"
      :width="520"
      v-model:visible="setReplayModalVisible"
      :can-backtest="true"
      :session-options="sessionOptions"
      :session-info="replayConfig.session_info"
      :begin-time="replayConfig.begin_time.split(' ')[1]"
      :end-time="
        replayConfig.end_time ? replayConfig.end_time.split(' ')[1] : ''
      "
      :log-level="replayConfig.log_level"
      @close="setReplayModalVisible = false"
      @confirm="(event) => handleReplayModal(event)"
    ></KfReplaySettingModal>
  </div>
</template>
<style lang="less">
.kf-strategy__warp {
  height: 100%;
}
</style>
