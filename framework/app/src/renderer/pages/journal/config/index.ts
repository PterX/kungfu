import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

type TableDataType = KungfuApi.FrameResolved & KungfuApi.SessionResolved;

const buildSorter =
  (dataIndex: keyof TableDataType) => (a: TableDataType, b: TableDataType) =>
    +Number(a[dataIndex]) - +Number(b[dataIndex]);

const buildStrSorter =
  (dataIndex: keyof TableDataType) => (a: TableDataType, b: TableDataType) =>
    a[dataIndex].toString().localeCompare(b[dataIndex].toString());

export const getSessionColumns = (): KfTradingDataTableHeaderConfig[] => [
  {
    type: 'string',
    name: 'name',
    dataIndex: 'name',
    width: 160,
    sorter: buildStrSorter('name'),
  },
  {
    type: 'string',
    name: t('journalConfig.begin_time'),
    dataIndex: 'begin_time_resolved',
    width: 160,
    sorter: buildSorter('begin_time'),
  },
  {
    type: 'string',
    name: t('journalConfig.end_time'),
    dataIndex: 'end_time_resolved',
    sorter: buildStrSorter('end_time'),
    width: 160,
  },
  {
    type: 'string',
    name: t('journalConfig.is_closed'),
    dataIndex: 'is_closed',
    width: 100,
  },
];

export const getFrameColumns = (): KfTradingDataTableHeaderConfig[] => [
  {
    type: 'string',
    name: t('journalConfig.gen_time'),
    dataIndex: 'gen_time_resolved',
    width: 160,
    sorter: buildSorter('genTime'),
  },
  {
    type: 'string',
    name: 'source → destination',
    dataIndex: 'source_to_dest',
    sorter: buildStrSorter('source_to_dest'),
    width: 260,
  },
  {
    type: 'string',
    name: t('journalConfig.msg_type'),
    dataIndex: 'stringMsgType',
    width: 140,
  },
];
