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
    title: t('remarks'),
    dataIndex: 'remarks',
    align: 'left',
    width: 80,
    fixed: 'left',
  },
  {
    title: t('operatorConfig.operator_file'),
    dataIndex: 'operatorFile',
    align: 'left',
    width: 120,
  },
  {
    title: t('operatorConfig.operator_ext'),
    dataIndex: 'operatorExt',
    align: 'left',
    width: 80,
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
      key: 'operator_id',
      name: t('operatorConfig.operator_id'),
      type: 'str',
      primary: true,
      required: true,
      tip: t('operatorConfig.operator_tip'),
    },
    {
      key: 'remarks',
      name: t('remarks'),
      type: 'str',
    },
    {
      key: 'file_path',
      name: t('operatorConfig.operator_path'),
      type: 'file',
      fileExtensions: ['py', 'pyd', 'so'],
      tip: t('operatorConfig.operator_path_tip'),
      required: true,
    },
  ],
};
