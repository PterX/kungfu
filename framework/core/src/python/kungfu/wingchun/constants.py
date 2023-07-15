#  SPDX-License-Identifier: Apache-2.0

from pykungfu import longfist as lf


class Exchange:
    SSE = "SSE"  #  上交所
    SZE = "SZE"  # 深交所
    SHFE = "SHFE"  # 上期所
    DCE = "DCE"  # 大商所
    CZCE = "CZCE"  # 郑商所
    CFFEX = "CFFEX"  # 中金所
    INE = "INE"  # 上海能源中心
    BSE = "BSE"  # 北交所
    GFEX = "GFEX"  # 广交所


class Region:
    CN = "CN"
    HK = "HK"


InstrumentType = lf.enums.InstrumentType
ExecType = lf.enums.ExecType
Side = lf.enums.Side
Offset = lf.enums.Offset
BsFlag = lf.enums.BsFlag
OrderStatus = lf.enums.OrderStatus
Direction = lf.enums.Direction
PriceType = lf.enums.PriceType
PriceLevel = lf.enums.PriceLevel
VolumeCondition = lf.enums.VolumeCondition
TimeCondition = lf.enums.TimeCondition
OrderActionFlag = lf.enums.OrderActionFlag
LedgerCategory = lf.enums.LedgerCategory
HedgeFlag = lf.enums.HedgeFlag
CommissionRateMode = lf.enums.CommissionRateMode
MarketType = lf.enums.MarketType
SubscribeDataType = lf.enums.SubscribeDataType
SubscribeInstrumentType = lf.enums.SubscribeInstrumentType
StrategyState = lf.enums.StrategyState
OperatorState = lf.enums.OperatorState
HistoryDataType = lf.enums.HistoryDataType

BasketVolumeType = lf.enums.BasketVolumeType
BasketType = lf.enums.BasketType

AllFinalOrderStatus = [
    int(OrderStatus.Filled),
    int(OrderStatus.Error),
    int(OrderStatus.PartialFilledNotActive),
    int(OrderStatus.Cancelled),
]

InstrumentTypeInStockAccount = [
    InstrumentType.Stock,
    InstrumentType.Bond,
    InstrumentType.Fund,
    InstrumentType.StockOption,
    InstrumentType.TechStock,
    InstrumentType.Index,
    InstrumentType.Repo,
]

ENUM_TYPES = [
    InstrumentType,
    ExecType,
    Side,
    Offset,
    BsFlag,
    OrderStatus,
    Direction,
    PriceType,
    VolumeCondition,
    TimeCondition,
    OrderActionFlag,
    LedgerCategory,
    HedgeFlag,
    MarketType,
    SubscribeDataType,
    SubscribeInstrumentType,
]
