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

export const getFrameColumns = (
  searchInUsing: boolean,
): KfTradingDataTableHeaderConfig[] => [
  {
    type: 'string',
    name: t('journalConfig.gen_time'),
    dataIndex: 'genTimeResolved',
    width: 128,
    textOverflow: 'ellipsis',
    sorter: buildSorter('genTime'),
  },
  {
    type: 'string',
    name: `${t('journalConfig.source')} → ${t('journalConfig.dest')}`,
    dataIndex: 'sourceToDest',
    textOverflow: 'ellipsis',
    sorter: buildStrSorter('sourceToDest'),
    width: 216,
  },
  {
    type: 'string',
    name: t('journalConfig.frame_id'),
    dataIndex: 'frameId',
    align: 'right',
    textOverflow: 'ellipsis',
    sorter: buildStrSorter('frameId'),

    width: 50,
  },
  {
    type: 'string',
    name: t('journalConfig.page_id'),
    dataIndex: 'pageId',
    align: 'right',
    textOverflow: 'ellipsis',
    sorter: buildStrSorter('pageId'),
    width: 40,
  },
  {
    type: 'string',
    name: t('journalConfig.msg_type'),
    dataIndex: 'msgTypeName',
    align: 'center',
    width: 180,
  },
  {
    type: 'string',
    name: t('journalConfig.msg_details'),
    dataIndex: 'dataAsString',
    textOverflow: searchInUsing ? 'clip' : 'ellipsis',
    wrap: searchInUsing,
    flex: 1,
  },
];

export const getReplayConfig = (
  startTime: string,
  endTime: string,
): KungfuApi.KfStrategyExtConfig => {
  return {
    type: [],
    name: t('strategyConfig.replay'),
    category: 'strategy',
    key: 'default',
    extPath: '',
    settings: [
      {
        key: 'start_time',
        name: t('strategyConfig.start_time'),
        type: 'timePicker',
        disabled: true,
      },
      {
        key: 'end_time',
        name: t('strategyConfig.end_time'),
        type: 'timePicker',
        abledTimeRange: [
          startTime ? startTime : '00:00:00',
          endTime ? endTime : '23:59:59',
        ],
      },
      {
        key: 'log_level',
        name: t('strategyConfig.log_level'),
        type: 'select',
        options: [
          { value: 'TRACE', label: 'TRACE' },
          { value: 'DEBUG', label: 'DEBUG' },
          { value: 'INFO', label: 'INFO' },
          { value: 'WARN', label: 'WARN' },
          { value: 'ERROR', label: 'ERROR' },
          { value: 'CRITICAL', label: 'CRITICAL' },
        ],
        default: 'INFO',
      },
    ],
  };
};
