<script setup lang="ts">
import { ref, computed, toRefs, onMounted, toRaw } from 'vue';
import { storeToRefs } from 'pinia';

import KfDashboard from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboard.vue';
import KfDashboardItem from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfDashboardItem.vue';
import KfProcessStatus from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfProcessStatus.vue';
import KfSetExtensionModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfSetExtensionModal.vue';
import KfSetByConfigModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfSetByConfigModal.vue';
import KfReplaySettingModal from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfReplaySettingModal.vue';
import FundTransModal from './FundTransModal.vue';
import Icon, {
  FileTextOutlined,
  SettingOutlined,
  DeleteOutlined,
  EyeOutlined,
  ReloadOutlined,
  PayCircleOutlined,
  HistoryOutlined,
} from '@ant-design/icons-vue';

import { categoryRegisterConfig, getColumns, getFundTransKey } from './config';
import {
  useTableSearchKeyword,
  handleOpenLogview,
  handleOpenJournalView,
  useDashboardBodySize,
  getInstrumentTypeColor,
  isInTdGroup,
  confirmModal,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  useAddUpdateRemoveKfConfig,
  handleSwitchProcessStatusGenerator,
  useSwitchAllConfig,
  useProcessStatusDetailData,
  useExtConfigsRelated,
  useCurrentGlobalKfLocation,
  useAllKfConfigData,
  useTdGroups,
  useAssets,
  useReplay,
  showTradingDataDetail,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import {
  getIfProcessRunning,
  getIfProcessStopping,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import {
  getIdByKfLocation,
  getProcessIdByKfLocation,
  ASSET_PRECISION,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  dealKfNumber,
  dealKfDecimalPrecision,
  buildTableColumnSorterWithStrike,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import KfBlinkNum from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfBlinkNum.vue';
import {
  addTdGroup,
  removeTdGroup,
  setTdGroup,
} from '@kungfu-trader/kungfu-js-api/actions';
import SetTdGroupModal from './SetTdGroupModal.vue';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';
import { messagePrompt } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { FundTransTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';

const { t } = VueI18n.global;
const { success, error } = messagePrompt();
const handleSwitchProcessStatus = handleSwitchProcessStatusGenerator();
const { dashboardBodyHeight, handleBodySizeChange } = useDashboardBodySize();
const { testCase } = storeToRefs(useGlobalStore());

globalThis.HookKeeper.getHooks().dealTradingData.register(
  {
    category: categoryRegisterConfig.category,
    group: '*',
    name: '*',
    mode: '*',
  },
  categoryRegisterConfig,
);

const setSourceModalVisible = ref<boolean>(false);
const setTdModalVisible = ref<boolean>(false);
const setTdConfigPayload = ref<KungfuApi.SetKfConfigPayload>({
  type: 'add',
  title: t('Td'),
  config: {} as KungfuApi.KfTdExtConfig,
});

const currentSelectedSourceId = ref<string>('');
const { extConfigs, tdExtTypeMap } = useExtConfigsRelated();
const { td } = toRefs(useAllKfConfigData());
const tdIdList = computed(() => {
  return td.value.map(
    (item: KungfuApi.KfLocation): string => `${item.group}-${item.name}`,
  );
});
const {
  dealRowClassName,
  customRow,
  currentGlobalKfLocation,
  resetCurrentGlobalKfLocation,
} = useCurrentGlobalKfLocation(window.watcher);

const { processStatusData, getProcessStatusName } =
  useProcessStatusDetailData();
const { allProcessOnline, handleSwitchAllProcessStatus } = useSwitchAllConfig(
  td,
  processStatusData,
);

const {
  replayConfig,
  setReplayModalVisible,
  sessionOptions,
  handleOpenReplayConfirmView,
  handleReplayModal,
} = useReplay();

const tdGroupDataLoaded = ref<boolean>(false);
const addTdGroupModalVisble = ref<boolean>(false);
const setTdGroupModalVisble = ref<boolean>(false);
const tdGroup = useTdGroups();
const tdGroupNames = computed(() => {
  return tdGroup.value.map((item) => item.name);
});

const addTdGroupConfigPayload = ref<KungfuApi.SetKfConfigPayload>({
  type: 'add',
  title: t('tdConfig.account_group'),
  config: {} as KungfuApi.KfTdExtConfig,
});

const currentAccout: {
  source: string;
  transfer_type: FundTransTypeEnum;
  config: KungfuApi.KfConfig | null;
  avail: number;
} = {
  source: '',
  transfer_type: FundTransTypeEnum.BetweenNodes,
  config: null,
  avail: 0,
};
const setFundTransModeModalVisible = ref<boolean>(false);
const setFundTransConfigModalVisible = ref<boolean>(false);
const setFundTransConfigPayload = ref<KungfuApi.SetKfConfigPayload>({
  type: 'custom',
  title: t('Td'),
  config: {} as KungfuApi.KfExtConfig,
});

const { searchKeyword, tableData } = useTableSearchKeyword<
  KungfuApi.KfConfig | KungfuApi.KfExtraLocation
>(td, ['group', 'name']);

const tableDataResolved = computed(() => {
  const tdGroupResolved: Record<string, KungfuApi.KfExtraLocation> = {};
  const tdResolved: KungfuApi.KfConfig[] = [];
  const markedNameToTdGroup: Record<string, KungfuApi.KfExtraLocation> = {};
  [...tdGroup.value, ...tableData.value].forEach((item) => {
    if ('children' in item) {
      markedNameToTdGroup[item.name] = { ...item };
      tdGroupResolved[item.name] = {
        ...item,
        key: item.name,
        children: [],
      };
      return;
    }

    const accountId = `${item.group}_${item.name}`;
    const targetGroupNames = Object.keys(markedNameToTdGroup).filter((name) => {
      const tdGroup = markedNameToTdGroup[name];
      return (tdGroup.children || []).includes(accountId);
    });
    if (targetGroupNames.length) {
      const targetGroupName = targetGroupNames[0];
      tdGroupResolved[targetGroupName].children?.push(item);
      return;
    }

    tdResolved.push(item as KungfuApi.KfConfig);
  });
  return [...Object.values(tdGroupResolved), ...tdResolved];
});

const hasTableData = computed(() => tableDataResolved.value.length > 0);

const { dealAssetPrecision, getAssetsByKfConfig, getAssetsByTdGroup } =
  useAssets();
const { handleConfirmAddUpdateKfConfig, handleRemoveKfConfig } =
  useAddUpdateRemoveKfConfig();

const columns = computed(() => {
  const sorter = (dataIndex: keyof KungfuApi.Asset) => {
    return buildTableColumnSorterWithStrike<
      KungfuApi.KfConfig,
      KungfuApi.Asset
    >('num', dataIndex, (kfConfig: KungfuApi.KfConfig) => {
      const { assets } = storeToRefs(useGlobalStore());
      const processId = getProcessIdByKfLocation(kfConfig);
      return assets.value[processId]
        ? assets.value[processId][dataIndex]
        : '--';
    });
  };

  const marginSorter = (dataIndex) => {
    return (a: KungfuApi.KfConfig, b: KungfuApi.KfConfig) => {
      return (
        (+Number(getAssetsByKfConfig(a)[dataIndex as keyof KungfuApi.Asset]) ||
          0) -
        (+Number(getAssetsByKfConfig(b)[dataIndex as keyof KungfuApi.Asset]) ||
          0)
      );
    };
  };

  if (!currentGlobalKfLocation.value) {
    return getColumns(
      {
        category: 'td',
        group: '*',
        name: '*',
        mode: '*',
      },
      sorter,
      marginSorter,
    );
  }

  return getColumns(currentGlobalKfLocation.value, sorter, marginSorter);
});

const customRowResolved = (
  record: KungfuApi.KfLocation | KungfuApi.KfConfig,
) => {
  if (record.category === 'tdGroup') {
    return customRow(record);
  }

  const assetGetter = () => {
    return dealAssetPrecision(getAssetsByKfConfig(record));
  };
  return {
    ...customRow(record),
    onMousedown: (event: MouseEvent) => {
      if (event.button === 2) {
        showTradingDataDetail(assetGetter, t('tdConfig.asset_details'), [], {
          collateral_ratio: (str) => {
            if (str === '--') {
              return str;
            } else {
              return ((Number(str) || 0) * 100).kfToFixed(1) + '%';
            }
          },
        });
      }
    },
  };
};

const getPrefixByLocation = (kfLocation: KungfuApi.KfLocation) =>
  globalThis.HookKeeper.getHooks().prefix.trigger(kfLocation);

const { setTdGroups } = useGlobalStore();

onMounted(() => {
  setTdGroups().then(() => {
    tdGroupDataLoaded.value = true;
  });
});

async function handleOpenSetTdModal(
  type = 'add' as KungfuApi.ModalChangeType,
  selectedSource: string,
  tdConfig?: KungfuApi.KfConfig,
) {
  const extConfig: KungfuApi.KfTdExtConfig = (extConfigs.value['td'] || {})[
    selectedSource
  ];

  if (!extConfig) {
    error(
      t('tdConfig.td_not_found', {
        td: selectedSource,
      }),
    );
    return;
  }

  currentSelectedSourceId.value = selectedSource;
  setTdConfigPayload.value.type = type;
  setTdConfigPayload.value.title = `${selectedSource} ${t('Td')}`;
  setTdConfigPayload.value.config =
    await globalThis.HookKeeper.getHooks().resolveExtConfig.trigger(
      {
        category: 'td',
        group: selectedSource,
        name: '*',
        mode: '*',
      },
      extConfig,
    );
  setTdConfigPayload.value.initValue = undefined;

  if (type === 'update') {
    if (tdConfig) {
      setTdConfigPayload.value.initValue = JSON.parse(tdConfig.value);
    }
  }

  if (!extConfig?.settings?.length) {
    error(t('tdConfig.sourse_not_found'));
    return;
  }

  setTdModalVisible.value = true;
}

function handleOpenSetSourceDialog() {
  setSourceModalVisible.value = true;
}

function handleOpenAddTdGroupDialog(type: KungfuApi.ModalChangeType) {
  addTdGroupConfigPayload.value.type = type;
  addTdGroupConfigPayload.value.config = {
    type: [],
    name: t('tdConfig.td_group'),
    category: 'tdGroup',
    key: 'TdGroup',
    silent: true,
    access: {},
    assets: {},
    extPath: '',
    version: '',
    description: '',
    dependencies: {},
    readmePath: '',
    releaseNotePath: '',
    settings: [
      {
        key: 'td_group_name',
        name: t('tdConfig.td_name'),
        type: 'str',
        primary: true,
        required: true,
        tip: t('tdConfig.need_only_group'),
      },
    ],
  };
  addTdGroupConfigPayload.value.initValue = undefined;
  addTdGroupModalVisble.value = true;
}

function handleFundTransModeDialog(config: KungfuApi.KfConfig) {
  if (getProcessStatusName(config) !== 'Ready') return;
  currentAccout.source = config.group;
  currentAccout.config = config;
  currentAccout.avail = dealKfDecimalPrecision(
    getAssetsByKfConfig(config).avail,
    ASSET_PRECISION,
  );
  setFundTransModeModalVisible.value = true;
}

function handleOpenSetFundTransModal(type: FundTransTypeEnum) {
  const extConfig: KungfuApi.KfTdExtConfig = (extConfigs.value['td'] || {})[
    currentAccout.source
  ];
  if (!extConfig || !extConfig.fundTrans) {
    error(
      t('fundTrans.config_error', {
        td: currentAccout.source,
      }),
    );
    return;
  }

  const selectFundTransConfig = extConfig.fundTrans[type];
  currentAccout.transfer_type = type;
  setTdConfigPayload.value.initValue = undefined;
  setFundTransConfigPayload.value.title = t('fundTrans.modal_title');
  setFundTransConfigPayload.value.config = {
    type: [],
    name: t('fundTrans.modal_title'),
    category: 'td',
    key: currentAccout.source,
    silent: true,
    access: {},
    assets: {},
    extPath: '',
    version: '',
    description: '',
    dependencies: {},
    readmePath: '',
    releaseNotePath: '',
    settings: selectFundTransConfig.settings,
  };

  setFundTransConfigModalVisible.value = true;
}

function handleConfirmFundTrans(formState) {
  const watcher = window.watcher as KungfuApi.Watcher;
  const formStateResolved = {
    ...formState,
    key: getFundTransKey(currentAccout.transfer_type),
    update_time: '',
  };

  const message: KungfuApi.TimeKeyValue = {
    key: watcher.now().toString(),
    update_time: watcher.now(),
    value: JSON.stringify(formStateResolved),
    tag_a: getFundTransKey(null),
    tag_b: '',
    tag_c: '',
    source: 0,
    dest: 0,
    uid_key: '',
  };
  const fundTransResult = watcher.issueCustomData(
    message,
    currentAccout.config as KungfuApi.KfConfig,
  );

  if (
    formState.source &&
    formState.target &&
    formState.source === formState.target
  ) {
    error('');
    return;
  }

  if (fundTransResult) {
    success();
  } else {
    error(t('fundTrans.tip_error'));
  }
}

function dealDisabledColor(config: KungfuApi.KfConfig) {
  return getProcessStatusName(config) === 'Ready'
    ? 'rgba(255, 255, 255, 1)'
    : 'rgba(255, 255, 255, 0.35)';
}

function handleConfirmAddUpdateTdGroup(
  formState: Record<string, KungfuApi.KfConfigValue>,
) {
  const { td_group_name } = formState;
  const newTdGroup: KungfuApi.KfExtraLocation = {
    category: 'tdGroup',
    group: 'group',
    name: td_group_name.toString(),
    mode: 'live',
    children: [],
  };

  return addTdGroup(newTdGroup)
    .then(() => {
      return setTdGroups();
    })
    .then(() => {
      success();
    })
    .catch((err) => {
      error(err.message || t('operation_failed'));
    });
}

function handleRemoveTdGroup(item: KungfuApi.KfExtraLocation) {
  confirmModal(
    t('tdConfig.delete_amount_group', {
      group: item.name,
    }),
    `${t('tdConfig.delete_amount_group', {
      group: item.name,
    })}, ${t('tdConfig.confirm_delete_group')}`,
  ).then((flag) => {
    if (!flag) return;
    removeTdGroup(item.name)
      .then(() => {
        return setTdGroups();
      })
      .then(() => {
        success();

        if (
          currentGlobalKfLocation.value &&
          getProcessIdByKfLocation(item) ===
            getProcessIdByKfLocation(currentGlobalKfLocation.value)
        ) {
          resetCurrentGlobalKfLocation();
        }
      })
      .catch((err) => {
        error(err.message || t('operation_failed'));
      });
  });
}

function handleRemoveTd(item: KungfuApi.KfConfig) {
  handleRemoveKfConfig(window.watcher, item, processStatusData.value)
    .then(() => {
      const accountId = getIdByKfLocation(item);
      const oldGroup = isInTdGroup(tdGroup.value, accountId);
      if (oldGroup) {
        const index = oldGroup.children?.indexOf(accountId);
        oldGroup.children.splice(index, 1);
        setTdGroup(toRaw(tdGroup.value)).then(() => {
          return setTdGroups();
        });
      }
    })
    .then(() => {
      success();
    })
    .catch((err) => {
      error(err.message || t('operation_failed'));
    });
}

function handleRequestPosition() {
  const res = window.watcher.requestPosition(window.watcher);
  if (res) {
    success(t('operation_success'));
  } else {
    error(t('operation_failed'));
  }
}

function isShowFundTransIcon(location: KungfuApi.KfConfig) {
  if (!location) return false;
  const extConfig: KungfuApi.KfTdExtConfig = (extConfigs.value['td'] || {})[
    location.group
  ];

  if (!extConfig || JSON.stringify(extConfig.fundTrans) === '{}') return false;

  return true;
}
</script>

<template>
  <div class="kf-td__warp kf-translateZ">
    <KfDashboard @boardSizeChange="handleBodySizeChange">
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
          <a-button size="small" @click="handleRequestPosition">
            <template #icon>
              <ReloadOutlined style="font-size: 14px" />
            </template>
          </a-button>
        </KfDashboardItem>
        <KfDashboardItem>
          <a-button size="small" @click="handleOpenAddTdGroupDialog('add')">
            {{ $t('tdConfig.add_group_placeholder') }}
          </a-button>
        </KfDashboardItem>
        <KfDashboardItem>
          <a-button
            size="small"
            type="primary"
            @click="handleOpenSetSourceDialog"
          >
            {{ $t('tdConfig.add_td') }}
          </a-button>
        </KfDashboardItem>
      </template>
      <a-table
        v-if="tdGroupDataLoaded"
        :class="{
          'has-data': hasTableData,
          'kf-ant-table': true,
          'kf-ant-table-sorter': true,
        }"
        :columns="columns"
        :data-source="tableDataResolved"
        size="small"
        :pagination="false"
        :scroll="{ y: dashboardBodyHeight - 4 }"
        :row-class-name="dealRowClassName"
        :customHeaderRow="
          (_column, _index) => {
            return {
              style: {
                whiteSpace: 'nowrap',
                overflow: 'hidden',
                textOverflow: 'ellipsis',
              },
            };
          }
        "
        :custom-row="customRowResolved"
        :default-expand-all-rows="true"
        :empty-text="$t('empty_text')"
      >
        <template
          #bodyCell="{
            column,
            record,
          }: {
            column: AntTableColumn,
            record: KungfuApi.KfConfig | KungfuApi.KfExtraLocation,
          }"
        >
          <template
            v-if="column.dataIndex === 'name' && record.category === 'td'"
          >
            <div class="td-name__warp">
              <a-tag
                :color="getInstrumentTypeColor(tdExtTypeMap[record.group])"
              >
                {{ record.group }}
              </a-tag>
              <span style="user-select: text">
                {{ record.name }}
              </span>
              <Icon
                v-if="getPrefixByLocation(record).prefixType === 'icon'"
                :component="getPrefixByLocation(record).prefix"
                style="font-size: 12px; margin-left: 7px"
              />
            </div>
          </template>
          <template
            v-else-if="
              column.dataIndex === 'name' && record.category === 'tdGroup'
            "
          >
            <div class="td-name__warp">
              <a-tag color="#FAAD14">{{ $t('tdConfig.account_group') }}</a-tag>
              <span>
                {{ record.name }}
              </span>
            </div>
          </template>
          <template
            v-else-if="
              column.dataIndex === 'accountName' && record.category === 'td'
            "
          >
            {{
              JSON.parse((record as KungfuApi.KfConfig).value).account_name ||
              '--'
            }}
          </template>

          <template v-else-if="column.dataIndex === 'stateStatus'">
            <KfProcessStatus
              v-if="record.category === 'td'"
              :status-name="getProcessStatusName(record)"
            ></KfProcessStatus>
          </template>
          <template v-else-if="column.dataIndex === 'processStatus'">
            <a-switch
              v-if="record.category === 'td'"
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
              @click="
                                (checked: boolean, Event: MouseEvent) => handleSwitchProcessStatus(checked, Event, record)
                            "
            ></a-switch>
          </template>
          <template v-else-if="column.dataIndex === 'unrealized_pnl'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              mode="compare-zero"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).unrealized_pnl,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              mode="compare-zero"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).unrealized_pnl,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'market_value'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).market_value,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).market_value,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'margin'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).margin ||
                    getAssetsByKfConfig(record).margin,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).margin ||
                    getAssetsByTdGroup(record).margin,
                  4,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'avail'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              mode="compare-zero"
              :num="
                dealKfNumber(getAssetsByKfConfig(record).avail, ASSET_PRECISION)
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(getAssetsByTdGroup(record).avail, ASSET_PRECISION)
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'frozen_cash'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              mode="compare-zero"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).frozen_cash,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).frozen_cash,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'total_asset'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              mode="compare-zero"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).total_asset,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).total_asset,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'short_cash'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              mode="compare-zero"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).short_cash,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).short_cash,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'collateral_ratio'">
            <span v-if="record.category === 'td'">
              {{
                getAssetsByKfConfig(record).collateral_ratio ||
                getAssetsByKfConfig(record).collateral_ratio === 0
                  ? `${(
                      getAssetsByKfConfig(record).collateral_ratio * 100
                    ).kfToFixed(1)}%`
                  : '--'
              }}
            </span>
            <span v-else-if="record.category === 'tdGroup'">{{ '--' }}</span>
          </template>
          <template v-else-if="column.dataIndex === 'avail_margin'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              mode="compare-zero"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).avail_margin,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).avail_margin,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'total_debt'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              mode="compare-zero"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).total_debt,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).total_debt,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'net_assets'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              mode="compare-zero"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).net_assets,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).net_assets,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'long_total_debt'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              mode="compare-zero"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).long_total_debt,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).long_total_debt,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'short_total_debt'">
            <KfBlinkNum
              v-if="record.category === 'td'"
              mode="compare-zero"
              :num="
                dealKfNumber(
                  getAssetsByKfConfig(record).short_total_debt,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
            <KfBlinkNum
              v-else-if="record.category === 'tdGroup'"
              :num="
                dealKfNumber(
                  getAssetsByTdGroup(record).short_total_debt,
                  ASSET_PRECISION,
                )
              "
            ></KfBlinkNum>
          </template>
          <template v-else-if="column.dataIndex === 'actions'">
            <div v-if="record.category === 'td'" class="kf-actions__warp">
              <HistoryOutlined
                v-if="testCase.replayEnabled[record.category]"
                style="font-size: 12px"
                @click.stop="
                  handleOpenReplayConfirmView(record as KungfuApi.KfConfig)
                "
              ></HistoryOutlined>
              <EyeOutlined
                style="font-size: 14px"
                @click.stop="handleOpenJournalView(record)"
              ></EyeOutlined>
              <!-- TODO -->
              <PayCircleOutlined
                v-if="isShowFundTransIcon(record as KungfuApi.KfConfig)"
                :style="{
                  color: dealDisabledColor(record as KungfuApi.KfConfig),
                }"
                @click.stop="
                  handleFundTransModeDialog(record as KungfuApi.KfConfig)
                "
              />
              <FileTextOutlined
                style="font-size: 12px"
                @click.stop="handleOpenLogview(record)"
              />
              <SettingOutlined
                style="font-size: 12px"
                @click.stop="
                  handleOpenSetTdModal(
                    'update',
                    record.group,
                    record as KungfuApi.KfConfig,
                  )
                "
              />
              <DeleteOutlined
                style="font-size: 12px"
                @click.stop="handleRemoveTd(record as KungfuApi.KfConfig)"
              />
            </div>
            <div v-if="record.category === 'tdGroup'" class="kf-actions__warp">
              <SettingOutlined
                style="font-size: 12px"
                @click.stop="setTdGroupModalVisble = true"
              />
              <DeleteOutlined
                style="font-size: 12px"
                @click.stop="handleRemoveTdGroup(record)"
              />
            </div>
          </template>
        </template>
      </a-table>
    </KfDashboard>
    <KfSetExtensionModal
      v-if="setSourceModalVisible"
      v-model:visible="setSourceModalVisible"
      extension-type="td"
      @confirm="handleOpenSetTdModal('add', $event)"
    ></KfSetExtensionModal>
    <FundTransModal
      v-if="setFundTransModeModalVisible"
      v-model:visible="setFundTransModeModalVisible"
      @confirm="handleOpenSetFundTransModal"
    ></FundTransModal>
    <KfSetByConfigModal
      v-if="setTdModalVisible"
      v-model:visible="setTdModalVisible"
      :payload="setTdConfigPayload"
      :primary-key-avoid-repeat-compare-target="tdIdList"
      :primary-key-avoid-repeat-compare-extra="currentSelectedSourceId"
      @confirm="
        handleConfirmAddUpdateKfConfig($event, 'td', currentSelectedSourceId)
      "
    ></KfSetByConfigModal>
    <KfSetByConfigModal
      v-if="addTdGroupModalVisble"
      v-model:visible="addTdGroupModalVisble"
      :payload="addTdGroupConfigPayload"
      :primary-key-avoid-repeat-compare-target="tdGroupNames"
      @confirm="({ formState }) => handleConfirmAddUpdateTdGroup(formState)"
    ></KfSetByConfigModal>
    <KfSetByConfigModal
      v-if="setFundTransConfigModalVisible"
      v-model:visible="setFundTransConfigModalVisible"
      :payload="setFundTransConfigPayload"
      @confirm="({ formState }) => handleConfirmFundTrans(formState)"
    ></KfSetByConfigModal>
    <SetTdGroupModal
      v-if="setTdGroupModalVisble"
      v-model:visible="setTdGroupModalVisble"
    ></SetTdGroupModal>
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
.kf-td__warp {
  height: 100%;

  .has-data .ant-table-tbody > tr > td:first-child > div {
    display: flow-root;
  }

  .td-name__warp {
    word-break: break-all;
  }
}
</style>
