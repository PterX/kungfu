<script setup lang="ts">
import { ref, computed, toRefs, Ref } from 'vue';

import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import AddOperatorModal from './AddOperatorModal.vue';
import KfSetByConfigModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfSetByConfigModal.vue';
import KfSetExtensionModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfSetExtensionModal.vue';
import Icon, {
  FileTextOutlined,
  SettingOutlined,
  DeleteOutlined,
  FormOutlined,
  BankOutlined,
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
  handleSwitchProcessStatus,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import {
  getIdByKfLocation,
  getConfigValue,
  getIfProcessRunning,
  getProcessIdByKfLocation,
  getIfProcessStopping,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { AddOperatorTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import path from 'path';

const { t } = VueI18n.global;
const { success, error } = messagePrompt();

const { dashboardBodyHeight, handleBodySizeChange } = useDashboardBodySize();
const { operator } = toRefs(useAllKfConfigData());
const operatorIdList = computed(() => {
  return operator.value.map((item: KungfuApi.KfLocation): string =>
    getIdByKfLocation(item),
  );
});
const { processStatusData } = useProcessStatusDetailData();
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

const columns = getColumns();

const addOperatorModalVisible = ref<boolean>(false);
const setOperatorModalVisible = ref<boolean>(false);
const setExtensionModalVisible = ref<boolean>(false);

const setOperatorConfigPayload = ref<KungfuApi.SetKfConfigPayload>({
  type: 'add',
  title: t('operatorConfig.operator'),
  config: {} as KungfuApi.KfExtConfig,
});

const getPrefixByLocation = (kfLocation: KungfuApi.KfLocation) =>
  globalThis.HookKeeper.getHooks().prefix.trigger(kfLocation);

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
  console.log(method, type, '---');
  switch (type) {
    case AddOperatorTypeEnum.Extension:
      handleOpenSetOperatorExtDialog();
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
  setOperatorConfigPayload.value.initValue = undefined;

  if (type === 'update' && operatorConfig) {
    setOperatorConfigPayload.value.initValue = JSON.parse(operatorConfig.value);
  }

  setOperatorModalVisible.value = true;
}

function handleOpenSetOperatorExtDialog() {
  setExtensionModalVisible.value = true;
}

function handleConfirmSetOperatorExtDialog(
  type = 'add' as KungfuApi.ModalChangeType,
  selectedSource: string,
  operatorConfig?: KungfuApi.KfConfig,
) {
  console.log(type, selectedSource, operatorConfig);
}

function getOperatorPathShowName(kfConfig: KungfuApi.KfConfig): string {
  const strategyPath = getConfigValue(kfConfig).file_path || '';
  return path.basename(strategyPath);
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
</script>

<template>
  <div class="kf-operator__warp kf-translateZ">
    <KfDashboard @bodySizeChange="handleBodySizeChange">
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
        class="kf-ant-table"
        ref="table"
        :columns="columns"
        :data-source="tableData"
        size="small"
        :pagination="false"
        :scroll="{ y: dashboardBodyHeight - 4 }"
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
          <template v-else-if="column.dataIndex === 'operatorFile'">
            {{ getOperatorPathShowName(record) }}
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
              <BankOutlined
                style="font-size: 12px"
                @click.stop="handleOpenJournalView(record)"
              ></BankOutlined>
              <FileTextOutlined
                style="font-size: 12px"
                @click.stop="handleOpenLogview(record)"
              />
              <FormOutlined
                style="font-size: 12px"
                @click.stop="handleOpenCodeView(record)"
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
      :width="420"
      v-model:visible="setOperatorModalVisible"
      :payload="setOperatorConfigPayload"
      :primaryKeyAvoidRepeatCompareTarget="operatorIdList"
      @confirm="handleConfirmAddUpdateKfConfig($event, 'operator', 'default')"
    ></KfSetByConfigModal>
    <KfSetExtensionModal
      v-if="setExtensionModalVisible"
      v-model:visible="setExtensionModalVisible"
      extensionType="operator"
      @confirm="handleConfirmSetOperatorExtDialog('add', $event)"
    ></KfSetExtensionModal>
  </div>
</template>
<style lang="less">
.kf-operator__warp {
  height: 100%;
}
</style>
