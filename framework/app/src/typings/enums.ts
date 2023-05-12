export enum KfLayoutDirection {
  v = 'v',
  h = 'h',
  unset = '',
}

export enum KfLayoutTargetDirectionClassName {
  unset = '',
  center = 'drag-over',
  top = 'drag-over-top',
  bottom = 'drag-over-bottom',
  left = 'drag-over-left',
  right = 'drag-over-right',
}

export enum MsgType {
  Position = 103,
  Quote = 401,
  Order = 202,
  OrderInput = 201,
  Trade = 203,
}
