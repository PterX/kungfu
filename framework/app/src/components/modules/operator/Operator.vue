<script setup lang="ts">
import { ref, computed, toRefs, Ref } from 'vue';

import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import AddOperatorModal from './AddOperatorModal.vue';
import KfSetByConfigModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfSetByConfigModal.vue';
import KfSetExtensionModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfSetExtensionModal.vue';

import {
  useTableSearchKeyword,
  useDashboardBodySize,
  //   handleOpenLogview,
  //   handleOpenJournalView,
  //   handleOpenCodeView,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { getColumns, setOperatorConfig } from './config';

import {
  useAllKfConfigData,
  useSwitchAllConfig,
  useProcessStatusDetailData,
  useAddUpdateRemoveKfConfig,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { getIdByKfLocation } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { AddOperatorTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const { t } = VueI18n.global;
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
const {
  handleConfirmAddUpdateKfConfig,
  // handleRemoveKfConfig
} = useAddUpdateRemoveKfConfig();

const columns = getColumns();

const addOperatorModalVisible = ref<boolean>(false);
const setOperatorModalVisible = ref<boolean>(false);
const setExtensionModalVisible = ref<boolean>(false);

const setOperatorConfigPayload = ref<KungfuApi.SetKfConfigPayload>({
  type: 'add',
  title: t('operatorConfig.operator'),
  config: {} as KungfuApi.KfExtConfig,
});

function handleOpenSetOperatorTypeDialog(type: AddOperatorTypeEnum) {
  console.log(type, '---');
  switch (type) {
    case AddOperatorTypeEnum.Extension:
      handleOpenSetOperatorExtDialog();
      break;
    case AddOperatorTypeEnum.File:
      handleOpenSetOperatorDialog('add');
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

  if (type === 'update') {
    if (operatorConfig) {
      setOperatorConfigPayload.value.initValue = JSON.parse(
        operatorConfig.value,
      );
    }
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
      ></a-table>
    </KfDashboard>
    <AddOperatorModal
      v-if="addOperatorModalVisible"
      v-model:visible="addOperatorModalVisible"
      @confirm="handleOpenSetOperatorTypeDialog"
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
