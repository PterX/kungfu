import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import {
  SideEnum,
  OffsetEnum,
  PriceTypeEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
const { t } = VueI18n.global;

export const getColumns = (): AntTableColumns => [
  {
    title: t('orderConfig.update_time'),
    dataIndex: 'update_time_resolved',
    align: 'left',
    width: 100,
  },
  {
    title: t('orderConfig.instrument_id'),
    dataIndex: 'instrument_id',
    align: 'left',
    width: 80,
  },
  {
    title: '',
    dataIndex: 'side',
    align: 'left',
    width: 40,
  },
  {
    title: '',
    dataIndex: 'offset',
    align: 'left',
    width: 40,
  },
  {
    title: t('orderConfig.limit_price'),
    dataIndex: 'limit_price_resolved',
    align: 'right',
    width: 120,
  },
  {
    title: t('orderConfig.entrust_volume'),
    dataIndex: 'volume',
    align: 'right',
    width: 120,
  },
  {
    title: t('orderConfig.time_condition'),
    dataIndex: 'time_condition',
    align: 'left',
    width: 120,
  },
  {
    title: t('orderConfig.parked_type'),
    dataIndex: 'parked_type',
    align: 'left',
    width: 120,
  },
  {
    title: t('orderConfig.order_status'),
    dataIndex: 'status',
    align: 'left',
    width: 120,
  },
  {
    title: t('orderConfig.dest_uname'),
    dataIndex: 'dest_uname',
    align: 'left',
    width: 80,
  },
];

export const getModalSettings = (): KungfuApi.KfConfigItem[] => {
  return [
    {
      key: 'embedded',
      name: t('tradingConfig.batch'),
      type: 'csvTable',
      importMode: 'reset',
      wrap: 'nowrap',
      template: [
        {
          name: '批量上传预埋单模版.csv',
          data: [
            {
              instrument_id: 'TS2309',
              exchange_id: 'CFFEX',
              side: 0,
              offset: 0,
              price_type: 0,
              limit_price: 11.46,
              volume: 1000,
            },
            {
              instrument_id: 'TS2309',
              exchange_id: 'CFFEX',
              side: 0,
              offset: 0,
              price_type: 0,
              volume: 1000,
              limit_price: 31.46,
            },
            {
              instrument_id: 'TS2309',
              exchange_id: 'CFFEX',
              side: 0,
              offset: 0,
              price_type: 0,
              volume: 1000,
              limit_price: 21.46,
            },
            {
              instrument_id: 'TS2309',
              exchange_id: 'CFFEX',
              side: 0,
              offset: 0,
              price_type: 0,
              volume: 1000,
              limit_price: 14.46,
            },
            {
              instrument_id: 'TS2309',
              exchange_id: 'CFFEX',
              side: 0,
              offset: 0,
              price_type: 0,
              volume: 1000,
              limit_price: 12.46,
            },
          ],
        },
      ],
      headers: [
        {
          title: 'instrument_id',
          description: t('tradingConfig.instrument_id_header_desc'),
          type: 'str',
          required: true,
        },
        {
          title: 'exchange_id',
          description: t('tradingConfig.exchange_id_header_desc'),
          type: 'str',
          required: true,
        },
        {
          title: 'side',
          description: t('tradingConfig.side_header_desc'),
          type: 'str',
          required: true,
        },
        {
          title: 'offset',
          description: t('tradingConfig.offset_header_desc'),
          type: 'str',
          required: true,
        },
        {
          title: 'price_type',
          description: t('tradingConfig.price_type_header_desc'),
          type: 'str',
          required: true,
        },
        {
          title: 'volume',
          description: t('tradingConfig.volume_header_desc'),
          type: 'str',
          required: true,
        },
        {
          title: 'limit_price',
          description: t('tradingConfig.limit_price_header_desc'),
          type: 'str',
          required: true,
        },
      ],
      default: [],
      noDivider: false,
      columns: [
        {
          key: 'instrument',
          name: t('tradingConfig.instrument'),
          type: 'instrument',
          required: true,
        },
        {
          key: 'side',
          name: t('tradingConfig.side'),
          type: 'select',
          options: [
            {
              label: t('tradingConfig.buy'),
              value: `${SideEnum.Buy}`,
            },
            {
              label: t('tradingConfig.sell'),
              value: `${SideEnum.Sell}`,
            },
          ],
          default: `${SideEnum.Buy}`,
          required: true,
        },
        {
          key: 'offset',
          name: t('tradingConfig.offset'),
          type: 'select',
          options: [
            {
              label: t('tradingConfig.open'),
              value: `${OffsetEnum.Open}`,
            },
            {
              label: t('tradingConfig.close'),
              value: `${OffsetEnum.Close}`,
            },
            {
              label: t('tradingConfig.close_today'),
              value: `${OffsetEnum.CloseToday}`,
            },
            {
              label: t('tradingConfig.close_yesterday'),
              value: `${OffsetEnum.CloseYest}`,
            },
          ],
          default: `${OffsetEnum.Open}`,
          required: true,
        },
        {
          key: 'price_type',
          name: t('tradingConfig.price_type'),
          type: 'select',
          options: [
            {
              label: t('tradingConfig.Limit'),
              value: `${PriceTypeEnum.Limit}`,
            },
            {
              label: t('tradingConfig.Market'),
              value: `${PriceTypeEnum.Market}`,
            },
            {
              label: t('tradingConfig.FakBest5'),
              value: `${PriceTypeEnum.FakBest5}`,
            },
            {
              label: t('tradingConfig.Forward_best'),
              value: `${PriceTypeEnum.ForwardBest}`,
            },
            {
              label: t('tradingConfig.Reverse_best'),
              value: `${PriceTypeEnum.ReverseBest}`,
            },
            {
              label: t('tradingConfig.Fak'),
              value: `${PriceTypeEnum.Fak}`,
            },
            {
              label: t('tradingConfig.Fok'),
              value: `${PriceTypeEnum.Fok}`,
            },
            {
              label: t('tradingConfig.EnhancedLimit'),
              value: `${PriceTypeEnum.EnhancedLimit}`,
            },
            {
              label: t('tradingConfig.AtAuctionLimit'),
              value: `${PriceTypeEnum.AtAuctionLimit}`,
            },
            {
              label: t('tradingConfig.AtAuction'),
              value: `${PriceTypeEnum.AtAuction}`,
            },
          ],
          default: `${PriceTypeEnum.Limit}`,
          required: true,
        },
        {
          key: 'volume',
          name: t('orderConfig.entrust_volume'),
          type: 'int',
          required: true,
          min: 1,
          default: 1,
        },
        {
          key: 'limit_price',
          name: t('orderConfig.limit_price'),
          type: 'float',
          required: true,
        },
      ],
    },
  ];
};
