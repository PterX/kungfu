namespace KfLayout {
  type ContentId = string;
  type BoardId = number;

  interface BoardInfo {
    paId: number;
    children?: number[];
    direction: KfLayoutDirection;
    contents?: ContentId[];
    current?: ContentId;
    width?: number | string;
    height?: number | string;
  }

  interface BoardsMap {
    [prop: BoardId]: BoardInfo;
  }

  interface ContentData {
    contentId: ContentId;
    boardId: BoardId;
  }
}
interface AntTableColumn {
  title: string;
  dataIndex: string;
  key?: string;
  width?: number | string;
  minWidth?: number | string;
  sorter?:
    | boolean
    | {
        compare: (
          a: any,
          b: any,
          sorterOrder: '' | 'ascend' | 'descend',
        ) => number;
      };
  align?: string;
  fixed?: string;
  defaultSortOrder?: string;
}

type AntTableColumns = Array<AntTableColumn>;
interface ExtraOrderInput {
  side: SideEnum;
  offset?: OffsetEnum;
  volume: number | bigint;
  price: number;
  accountId?: string;
}

interface KfTradingDataTableHeaderConfig {
  name: string;
  dataIndex: string;
  align?: 'left' | 'right' | 'center';
  width?: number;
  flex?: number;
  textOverflow?: 'visible' | 'hidden' | 'ellipsis' | 'clip';
  wrap?: boolean;
  type?:
    | 'number'
    | 'string'
    | 'source'
    | 'nanoTime'
    | 'exchange'
    | 'offset'
    | 'side'
    | 'priceType'
    | 'direction'
    | 'actions';
  sorter?: (a: any, b: any, sorterOrder: '' | 'ascend' | 'descend') => number;
}

type KfTradingDataTableSelection = Record<
  string,
  {
    disabled?: boolean;
  }
>;

type BuiltinComponents =
  | 'Pos'
  | 'PosGlobal'
  | 'Order'
  | 'Trade'
  | 'Td'
  | 'Md'
  | 'Strategy'
  | 'Operator'
  | 'TradingTask'
  | 'MarketData'
  | 'OrderBook'
  | 'MakeOrder'
  | 'FutureArbitrage'
  | 'BlockTrade'
  | 'OrderTriggerRecord'
  | 'TransferRecord';

interface BuiltinComponentPropsMap {
  TradingTask?: {
    taskFilter?: (task: Pm2ProcessStatusDetail) => boolean;
    taskSorter?: (
      a: Pm2ProcessStatusDetail,
      b: Pm2ProcessStatusDetail,
    ) => number;
    strategyFilter?: (
      strategyExtConfig: KungfuApi.KfStrategyExtConfig,
    ) => boolean;
  };
  MakeOrder?: {
    sideFilter?: (instrumentType: InstrumentTypeEnum) => string[];
  };
}

// interface BuiltinComponentInjectKeysMap{
//   Side:{
//     sideFilter?: (instrumentType: InstrumentTypeEnum) => string[];
//   }
// }

declare module 'worker-loader!*' {
  class WebpackWorker extends Worker {
    constructor();
  }

  export = WebpackWorker;
}

declare module '*.svg' {
  const content;
  export default content;
}
