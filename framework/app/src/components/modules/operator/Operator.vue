<script setup lang="ts">
import { ref, computed, toRefs, Ref } from 'vue';

import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import AddOperatorModal from './AddOperatorModal.vue';
import KfSetByConfigModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfSetByConfigModal.vue';
import KfSetExtensionModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfSetExtensionModal.vue';
import KfReplaySettingModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfReplaySettingModal.vue';
import KfProcessStatus from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfProcessStatus.vue';
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
  handleOpenJournalView,
  handleOpenLogview,
  handleOpenCodeView,
  messagePrompt,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { getColumns, setOperatorConfig } from './config';

import {
  useAllKfConfigData,
  useSwitchAllConfig,
  useProcessStatusDetailData,
  useAddUpdateRemoveKfConfig,
  handleSwitchProcessStatusGenerator,
  useExtConfigsRelated,
  useReplay,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import {
  getIfProcessRunning,
  getIfProcessStopping,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import {
  getIdByKfLocation,
  getProcessIdByKfLocation,
  getConfigValue,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';

import { AddOperatorTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import path from 'path';

const { t } = VueI18n.global;
const { success, error } = messagePrompt();
const { dashboardBodyHeight, handleBodySizeChange } = useDashboardBodySize();
const { operator } = toRefs(useAllKfConfigData());
const operatorIdList = computed(() => {
  return operator.value.map((item: KungfuApi.KfLocation): string =>
    item.group === 'default' ? item.name : `${item.group}-${item.name}`,
  );
});
const { processStatusData, getProcessStatusName } =
  useProcessStatusDetailData();
const { allProcessOnline, handleSwitchAllProcessStatus } = useSwitchAllConfig(
  operator,
  processStatusData,
);

const { searchKeyword, tableData } = useTableSearchKeyword<KungfuApi.KfConfig>(
  operator as Ref<KungfuApi.KfConfig[]>,
  ['name'],
);
const { handleConfirmAddUpdateKfConfig, handleRemoveKfConfig } =
  useAddUpdateRemoveKfConfig();

const {
  replayConfig,
  setReplayModalVisible,
  sessionOptions,
  handleOpenReplayConfirmView,
  handleReplayModal,
} = useReplay();

const columns = getColumns();
const handleSwitchProcessStatus = handleSwitchProcessStatusGenerator();

const addOperatorModalVisible = ref<boolean>(false);
const setOperatorModalVisible = ref<boolean>(false);
const setOperatorByExtModalVisible = ref<boolean>(false);
const setExtensionModalVisible = ref<boolean>(false);
const currentSelectedExtKey = ref<string>('');

const setOperatorConfigPayload = ref<KungfuApi.SetKfConfigPayload>({
  type: 'add',
  title: t('operatorConfig.operator'),
  config: {} as KungfuApi.KfOperatorExtConfig,
});

const getPrefixByLocation = (kfLocation: KungfuApi.KfLocation) =>
  globalThis.HookKeeper.getHooks().prefix.trigger(kfLocation);

const { extConfigs } = useExtConfigsRelated();

function getOperatorType(
  operatorConfig: KungfuApi.KfConfig,
): AddOperatorTypeEnum {
  if (operatorConfig.group === 'default') {
    return AddOperatorTypeEnum.File;
  } else {
    return AddOperatorTypeEnum.Extension;
  }
}

function handleOpenSetOperatorTypeDialog(
  method: KungfuApi.ModalChangeType,
  type: AddOperatorTypeEnum,
  operatorConfig?: KungfuApi.KfConfig,
) {
  switch (type) {
    case AddOperatorTypeEnum.Extension:
      if (method === 'add') {
        handleOpenSetOperatorExtDialog();
      } else if (method === 'update' && operatorConfig) {
        handleConfirmSetOperatorExtDialog(
          'update',
          operatorConfig.group,
          operatorConfig,
        );
      }
      break;
    case AddOperatorTypeEnum.File:
      handleOpenSetOperatorDialog(method, operatorConfig);
      break;
  }
}

function handleOpenSetOperatorDialog(
  type: KungfuApi.ModalChangeType,
  operatorConfig?: KungfuApi.KfConfig,
) {
  setOperatorConfigPayload.value.type = type;
  setOperatorConfigPayload.value.config = setOperatorConfig;
  setOperatorConfigPayload.value.title = t('operatorConfig.operator');
  setOperatorConfigPayload.value.initValue = undefined;

  if (type === 'update' && operatorConfig) {
    setOperatorConfigPayload.value.initValue = JSON.parse(operatorConfig.value);
  }

  setOperatorModalVisible.value = true;
}

function handleOpenSetOperatorExtDialog() {
  setExtensionModalVisible.value = true;
}

async function handleConfirmSetOperatorExtDialog(
  type = 'add' as KungfuApi.ModalChangeType,
  selectedOperatorExtKey: string,
  operatorConfig?: KungfuApi.KfConfig,
) {
  const extConfig: KungfuApi.KfOperatorExtConfig = (extConfigs.value[
    'operator'
  ] || {})[selectedOperatorExtKey];

  if (!extConfig) {
    error(
      `${selectedOperatorExtKey} ${t('operatorConfig.operator_not_found', {
        md: selectedOperatorExtKey,
      })}`,
    );
    return;
  }

  currentSelectedExtKey.value = selectedOperatorExtKey;
  setOperatorConfigPayload.value.type = type;
  setOperatorConfigPayload.value.title = `${selectedOperatorExtKey} ${t(
    'Operator',
  )}`;
  setOperatorConfigPayload.value.config =
    await globalThis.HookKeeper.getHooks().resolveExtConfig.trigger(
      {
        category: 'operator',
        group: selectedOperatorExtKey,
        name: '*',
        mode: '*',
      },
      extConfig,
    );
  setOperatorConfigPayload.value.initValue = undefined;

  if (type === 'update' && operatorConfig) {
    setOperatorConfigPayload.value.initValue = JSON.parse(operatorConfig.value);
  }

  if (!extConfig?.settings?.length) {
    handleConfirmAddUpdateKfConfig(
      {
        formState: {} as Record<string, KungfuApi.KfConfigValue>,
        configSettings: [],
        idByPrimaryKeys: selectedOperatorExtKey,
        changeType: type,
      },
      'operator',
      selectedOperatorExtKey,
    );
    return;
  }

  setOperatorByExtModalVisible.value = true;
}

function getOperatorPathShowName(kfConfig: KungfuApi.KfConfig): string {
  const configValue = getConfigValue(kfConfig);
  if (!configValue || !configValue.file_path) return '--';

  const strategyPath = configValue.file_path;
  return path.basename(strategyPath);
}

function getOperatorExtShowName(kfConfig: KungfuApi.KfConfig): string {
  if (kfConfig.group === 'default') return '--';

  return extConfigs.value.operator[kfConfig.group]?.name || kfConfig.group;
}

function handleRemoveOperator(record: KungfuApi.KfConfig) {
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
  <div class="kf-operator__warp kf-translateZ">
    <KfDashboard @bodySizeChange="handleBodySizeChange">
      <template #header>
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
        <KfDashboardItem>
          <a-button
            size="small"
            type="primary"
            @click="addOperatorModalVisible = true"
          >
            {{ $t('operatorConfig.add_operator') }}
          </a-button>
        </KfDashboardItem>
      </template>
      <a-table
        ref="table"
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
            record: KungfuApi.KfConfig,
          }"
        >
          <template v-if="column.dataIndex === 'name'">
            <span>{{ getIdByKfLocation(record) }}</span>
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
          <template v-else-if="column.dataIndex === 'operatorFile'">
            {{ getOperatorPathShowName(record) }}
          </template>
          <template v-else-if="column.dataIndex === 'operatorExt'">
            {{ getOperatorExtShowName(record) }}
          </template>
          <template v-else-if="column.dataIndex === 'stateStatus'">
            <KfProcessStatus
              :status-name="getProcessStatusName(record)"
            ></KfProcessStatus>
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
                @click.stop="
                  handleOpenSetOperatorTypeDialog(
                    'update',
                    getOperatorType(record),
                    record,
                  )
                "
              />
              <DeleteOutlined
                style="font-size: 12px"
                @click.stop="handleRemoveOperator(record)"
              />
            </div>
          </template>
        </template>
      </a-table>
    </KfDashboard>
    <AddOperatorModal
      v-if="addOperatorModalVisible"
      v-model:visible="addOperatorModalVisible"
      @confirm="handleOpenSetOperatorTypeDialog('add', $event)"
    ></AddOperatorModal>
    <KfSetByConfigModal
      v-if="setOperatorModalVisible"
      v-model:visible="setOperatorModalVisible"
      :width="420"
      :payload="setOperatorConfigPayload"
      :primary-key-avoid-repeat-compare-target="operatorIdList"
      @confirm="handleConfirmAddUpdateKfConfig($event, 'operator', 'default')"
    ></KfSetByConfigModal>
    <KfSetExtensionModal
      v-if="setExtensionModalVisible"
      v-model:visible="setExtensionModalVisible"
      extension-type="operator"
      @confirm="handleConfirmSetOperatorExtDialog('add', $event)"
    ></KfSetExtensionModal>
    <KfSetByConfigModal
      v-if="setOperatorByExtModalVisible"
      v-model:visible="setOperatorByExtModalVisible"
      :payload="setOperatorConfigPayload"
      :primary-key-avoid-repeat-compare-target="operatorIdList"
      :primary-key-avoid-repeat-compare-extra="currentSelectedExtKey"
      @confirm="
        handleConfirmAddUpdateKfConfig(
          $event,
          'operator',
          currentSelectedExtKey,
        )
      "
    ></KfSetByConfigModal>
    <KfReplaySettingModal
      v-if="setReplayModalVisible"
      :width="720"
      v-model:visible="setReplayModalVisible"
      :session-options="sessionOptions"
      :session-info="replayConfig.session_info"
      :begin-time="replayConfig.begin_time"
      :end-time="replayConfig.end_time ? replayConfig.end_time : ''"
      :log-level="replayConfig.log_level"
      @close="setReplayModalVisible = false"
      @confirm="(event) => handleReplayModal(event)"
    ></KfReplaySettingModal>
  </div>
</template>
<style lang="less">
.kf-operator__warp {
  height: 100%;
}
</style>
