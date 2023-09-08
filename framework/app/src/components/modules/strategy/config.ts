import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

export const getColumns = (
  sorter: (
    dataIndex: string,
  ) => (a: KungfuApi.KfConfig, b: KungfuApi.KfConfig) => number,
): AntTableColumns => [
  {
    title: t('strategyConfig.strategy_id'),
    dataIndex: 'name',
    align: 'left',
    width: 90,
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
    title: t('strategyConfig.marked_value'),
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
  extPath: '',
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
      key: 'file_path',
      name: t('strategyConfig.strategy_path'),
      type: 'file',
      tip: t('strategyConfig.strategy_path_tip'),
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
        type: 'str',
        disabled: true,
      },
      {
        key: 'end_time',
        name: t('strategyConfig.end_time'),
        type: 'str',
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
