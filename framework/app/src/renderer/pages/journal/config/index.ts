import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

type TableDataType = KungfuApi.FrameResolved & KungfuApi.SessionResolved;

const buildSorter =
  (dataIndex: keyof TableDataType) => (a: TableDataType, b: TableDataType) =>
    +Number(a[dataIndex]) - +Number(b[dataIndex]);

const buildStrSorter =
  (dataIndex: keyof TableDataType) => (a: TableDataType, b: TableDataType) =>
    a[dataIndex].toString().localeCompare(b[dataIndex].toString());

export const getSessionColumns = (): AntTableColumn[] => [
  {
    title: t('journalConfig.session_id'),
    dataIndex: 'sessionName',
    align: 'left',
    width: 240,
    sorter: {
      compare: buildStrSorter('sessionName'),
    },
    fixed: 'left',
  },
  {
    title: t('journalConfig.begin_time'),
    dataIndex: 'beginTimeResolved',
    align: 'left',
    width: 160,
    sorter: {
      compare: buildSorter('begin_time'),
    },
    fixed: 'left',
  },
  {
    title: t('journalConfig.end_time'),
    dataIndex: 'endTimeResolved',
    width: 160,
    align: 'left',
    sorter: { compare: buildStrSorter('end_time') },
    fixed: 'left',
  },
  {
    title: t('journalConfig.status'),
    dataIndex: 'status',
    align: 'left',
    fixed: 'right',
  },
];

export const SessionStatus: Record<
  SessionStatusEnum,
  KungfuApi.KfTradeValueCommonData
> = {
  [SessionStatusEnum.Running]: {
    name: t('journalConfig.running'),
    color: '#FAAD14',
  },
  [SessionStatusEnum.Finished]: {
    name: t('journalConfig.finished'),
    color: 'gray',
  },
};

export const getFrameColumns = (): KfTradingDataTableHeaderConfig[] => [
  {
    type: 'string',
    name: t('journalConfig.gen_time'),
    dataIndex: 'genTimeResolved',
    width: 160,
    overflow: 'ellipsis',
    sorter: buildSorter('genTime'),
  },
  {
    type: 'string',
    name: `${t('journalConfig.source')} → ${t('journalConfig.dest')}`,
    dataIndex: 'sourceToDest',
    overflow: 'ellipsis',
    sorter: buildStrSorter('sourceToDest'),
    width: 280,
  },
  {
    type: 'string',
    name: t('journalConfig.frame_id'),
    dataIndex: 'frameId',
    align: 'right',
    overflow: 'ellipsis',
    sorter: buildStrSorter('frameId'),

    width: 70,
  },
  {
    type: 'string',
    name: t('journalConfig.page_id'),
    dataIndex: 'pageId',
    align: 'right',
    overflow: 'ellipsis',
    sorter: buildStrSorter('pageId'),
    width: 50,
  },
  {
    type: 'string',
    name: t('journalConfig.msg_type'),
    dataIndex: 'msgTypeName',
    width: 190,
  },
  {
    type: 'string',
    name: t('journalConfig.msg_details'),
    dataIndex: 'dataAsString',
    overflow: 'ellipsis',
    flex: 1,
  },
];
