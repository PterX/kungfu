import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

export const getColumns = (
  sorter: (
    dataIndex: keyof KungfuApi.KfConfig | keyof KungfuApi.Asset,
  ) => (
    a: KungfuApi.KfConfig,
    b: KungfuApi.KfConfig,
    sorterOrder: '' | 'ascend' | 'descend',
  ) => number,
): AntTableColumns => [
  {
    title: t('strategyConfig.strategy_id'),
    dataIndex: 'name',
    align: 'left',
    width: 90,
    fixed: 'left',
  },
  {
    title: t('remarks'),
    dataIndex: 'remarks',
    align: 'left',
    width: 120,
    fixed: 'left',
  },
  {
    title: t('strategyConfig.strategy_file'),
    dataIndex: 'strategyFile',
    align: 'left',
    width: 120,
    fixed: 'left',
  },
  {
    title: t('strategyConfig.process_status'),
    dataIndex: 'processStatus',
    align: 'center',
    width: 60,
    fixed: 'left',
  },
  {
    title: t('strategyConfig.unrealized_pnl'),
    dataIndex: 'unrealizedPnl',
    align: 'right',
    sorter: {
      compare: sorter('unrealized_pnl'),
    },
    width: 110,
  },
  {
    title: t('strategyConfig.market_value'),
    dataIndex: 'marketValue',
    align: 'right',
    sorter: {
      compare: sorter('market_value'),
    },
    width: 110,
  },
  {
    title: t('strategyConfig.actions'),
    dataIndex: 'actions',
    align: 'right',
    width: 160,
    fixed: 'right',
  },
];

export const setStrategyConfig: KungfuApi.KfStrategyExtConfig = {
  type: [],
  name: t('strategyConfig.strategy'),
  category: 'strategy',
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
      key: 'strategy_id',
      name: t('strategyConfig.strategy_id'),
      type: 'str',
      primary: true,
      required: true,
      tip: t('strategyConfig.strategy_tip'),
    },
    {
      key: 'remarks',
      name: t('remarks'),
      type: 'str',
    },
    {
      key: 'file_path',
      name: t('strategyConfig.strategy_path'),
      type: 'file',
      fileExtensions: ['py', 'pyd', 'so'],
      tip: t('strategyConfig.strategy_path_tip'),
      required: true,
    },
  ],
};
