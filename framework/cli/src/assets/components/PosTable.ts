import { DirectionEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import {
  dealKfNumber,
  dealKfDecimalPrecision,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  dealDirection,
  getPrecisionByInstrumentType,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
import colors from 'colors';
import { calcHeaderWidth, colorNum, parseToString } from '../methods/utils';
import Table from './Table';

export class PosTable extends Table {
  constructor() {
    super();
    this.headers = [
      'Ticker',
      'Dir',
      'Yesterday',
      'Today',
      'Total',
      'Open',
      'Last',
      'UnrealPnl',
    ];
    this.columnWidth = [8, 4, 9, 9, 9, 9, 9, 12];
  }

  /**
   * @param  {Object} accountData
   * @param  {Object} processStatus
   */

  setItems(positions: KungfuApi.PositionResolved[]) {
    this.refresh(positions);
  }

  dealDirection(direction: DirectionEnum) {
    const name = dealDirection(direction).name;
    if (direction === DirectionEnum.Long) {
      return colors.red(name);
    } else {
      return colors.green(name);
    }
  }

  refresh(positions: KungfuApi.PositionResolved[]) {
    const posListData = positions.map((p: KungfuApi.PositionResolved) => {
      const direction = this.dealDirection(p.direction);
      const precision = getPrecisionByInstrumentType(p.instrument_type);
      return parseToString(
        [
          p.instrument_id_resolved,
          direction,
          p.yesterday_volume,
          dealKfDecimalPrecision(p.volume - p.yesterday_volume, precision),
          p.volume,
          dealKfNumber(p.avg_open_price, precision),
          dealKfNumber(p.last_price, precision),
          colorNum(dealKfNumber(p.unrealized_pnl, precision)),
        ],
        calcHeaderWidth(this.headers, this.columnWidth),
        this.pad,
      );
    });
    this.table && this.table.setItems(posListData);
    if (this.table && !this.table.childList.focused) {
      this.table.childList.setScrollPerc(0);
    }
  }
}

export default () => new PosTable();
