import { AddOperatorType } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { AddOperatorTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
const { t } = VueI18n.global;

export const getColumns = (): AntTableColumns => [
  {
    title: t('operatorConfig.operator_id'),
    dataIndex: 'name',
    align: 'left',
    width: 90,
    fixed: 'left',
  },
  {
    title: t('operatorConfig.operator_file'),
    dataIndex: 'operatorFile',
    align: 'left',
    width: 120,
    fixed: 'left',
  },
  {
    title: t('operatorConfig.state_status'),
    dataIndex: 'stateStatus',
    align: 'left',
    width: 80,
  },
  {
    title: t('operatorConfig.process_status'),
    dataIndex: 'processStatus',
    align: 'center',
    width: 60,
    fixed: 'left',
  },
  {
    title: t('operatorConfig.actions'),
    dataIndex: 'actions',
    align: 'right',
    width: 160,
    fixed: 'right',
  },
];

export const addOperatorConfig: KungfuApi.KfConfigItem = {
  key: 'type',
  name: '类型',
  default: AddOperatorTypeEnum.Extension,
  type: 'radio',
  options: [
    {
      label: AddOperatorType[AddOperatorTypeEnum.Extension].name,
      value: AddOperatorTypeEnum.Extension,
    },
    {
      label: AddOperatorType[AddOperatorTypeEnum.File].name,
      value: AddOperatorTypeEnum.File,
    },
  ],
};

export const setOperatorConfig: KungfuApi.KfOperatorExtConfig = {
  type: [],
  name: t('operatorConfig.operator'),
  category: 'operator',
  key: 'default',
  extPath: '',
  settings: [
    {
      key: 'operator_id',
      name: t('operatorConfig.operator_id'),
      type: 'str',
      primary: true,
      required: true,
      tip: t('operatorConfig.operator_tip'),
    },
    {
      key: 'file_path',
      name: t('operatorConfig.operator_path'),
      type: 'file',
      tip: t('operatorConfig.operator_path_tip'),
      required: true,
    },
  ],
};

export const getReplayConfig = (
  begintime: string,
  endTime: string,
): KungfuApi.KfStrategyExtConfig => {
  console.log(begintime, endTime);
  return {
    type: [],
    name: t('strategyConfig.replay'),
    category: 'strategy',
    key: 'default',
    extPath: '',
    settings: [
      {
        key: 'begin_time',
        name: t('strategyConfig.begin_time'),
        type: 'timePicker',
        disabled: true,
      },
      {
        key: 'end_time',
        name: t('strategyConfig.end_time'),
        type: 'timePicker',
      },
      {
        key: 'log_level',
        name: t('strategyConfig.log_level'),
        type: 'select',
        options: [
          { value: '-l trace', label: 'TRACE' },
          { value: '-l debug', label: 'DEBUG' },
          { value: '-l info', label: 'INFO' },
          { value: '-l warning', label: 'WARN' },
          { value: '-l error', label: 'ERROR' },
          { value: '-l critical', label: 'CRITICAL' },
        ],
        default: '-l info',
      },
    ],
  };
};
