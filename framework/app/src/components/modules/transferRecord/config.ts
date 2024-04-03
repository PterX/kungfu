import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

export const getColumns = (): AntTableColumns => [
  {
    title: t('fundTrans.update_time'),
    dataIndex: 'update_time',
    align: 'center',
    width: 120,
  },
  {
    title: t('fundTrans.status'),
    dataIndex: 'status',
    align: 'center',
    width: 80,
  },
  {
    title: t('fundTrans.source'),
    dataIndex: 'source',
    align: 'center',
    width: 80,
  },
  {
    title: t('fundTrans.target'),
    dataIndex: 'target',
    align: 'center',
    width: 80,
  },
  {
    title: t('fundTrans.trans_type'),
    dataIndex: 'trans_type',
    align: 'center',
    width: 120,
  },
  {
    title: t('fundTrans.amount'),
    dataIndex: 'amount',
    align: 'center',
    width: 80,
  },
];
