import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { CURRENT_STAGE } from '@kungfu-trader/kfx-ui-login-authing/src/configs/authing';
import { WalletTransTypeEnum } from '../typings/enum';
import { WalletTransData } from '../typings';

const { t } = VueI18n.global;

const rechargeUrlPrefix = CURRENT_STAGE === 'prod' ? 'www' : CURRENT_STAGE;

export const RechargeUrl = `https://${rechargeUrlPrefix}.kungfu-trader.com/index.php/my-account-2/orders/`;

export const LocalStorageKeys = {
  LoginForm: 'AuthingLoginFrom',
} as const;

export const AwsWalletKeys = {
  ShowRecords: 'aws-wallet:show-records',
  ToRecharge: 'aws-wallet:recharge',
  RefreshData: 'aws-wallet:refresh-data',
} as const;

const buildTableColumnSorterWithStrike = <T, U = object>(
  type: 'num' | 'str',
  dataIndex: keyof T | keyof U,
  transform?: (data: T) => number | string | null,
) => {
  return (a: T, b: T, sorterOrder: '' | 'ascend' | 'descend') => {
    if (type === 'num') {
      let aVal = (transform ? transform(a) : a[dataIndex as keyof T]) ?? '--',
        bVal = (transform ? transform(b) : b[dataIndex as keyof T]) ?? '--';
      if (sorterOrder === 'ascend') {
        aVal = aVal === '--' ? Infinity : aVal;
        bVal = bVal === '--' ? Infinity : bVal;
      } else if (sorterOrder === 'descend') {
        aVal = aVal === '--' ? -Infinity : aVal;
        bVal = bVal === '--' ? -Infinity : bVal;
      } else {
        return 0;
      }
      return Number(aVal) - Number(bVal);
    } else {
      return `${
        (transform ? transform(a) : a[dataIndex as keyof T]) ?? ''
      }`.localeCompare(
        `${(transform ? transform(b) : b[dataIndex as keyof T]) ?? ''}`,
      );
    }
  };
};

export const TransRecordsTableColumns: AntTableColumn[] = [
  {
    title: t('awsWallet.transTime'),
    dataIndex: 'time',
    width: '20%',
    align: 'left',
    sorter: {
      compare: buildTableColumnSorterWithStrike<WalletTransData>(
        'num',
        'settled_at',
      ),
    },
  },
  {
    title: t('awsWallet.transType'),
    dataIndex: 'transaction_type',
    width: '20%',
    align: 'left',
  },
  // {
  //   title: t('awsWallet.transNote'),
  //   dataIndex: 'note',
  //   width: '20%',
  //   align: 'left',
  // },
  {
    title: t('awsWallet.transAmount'),
    dataIndex: 'amount',
    width: '20%',
    align: 'right',
  },
  // {
  //   title: t('awsWallet.balance'),
  //   dataIndex: 'balance',
  //   width: '20%',
  //   align: 'right',
  // },
];

export const transTypeConfig: Record<
  WalletTransTypeEnum,
  KungfuApi.KfTradeValueCommonData
> = {
  [WalletTransTypeEnum.Inbound]: {
    name: t('awsWallet.recharge'),
    color: 'primary' as KungfuApi.AntInKungfuColorTypes,
  },
  [WalletTransTypeEnum.Outbound]: {
    name: t('awsWallet.cost'),
    color: 'green',
  },
};
