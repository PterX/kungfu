class SingletonDecimalFormatter {
  private static instance: SingletonDecimalFormatter;
  private formattersMap: Map<number, Intl.NumberFormat>;

  private constructor() {
    this.formattersMap = new Map();
  }

  public static getInstance(): SingletonDecimalFormatter {
    if (!SingletonDecimalFormatter.instance) {
      SingletonDecimalFormatter.instance = new SingletonDecimalFormatter();
    }
    return SingletonDecimalFormatter.instance;
  }

  private getFormatter(precision: number): Intl.NumberFormat {
    if (!this.formattersMap.has(precision)) {
      this.formattersMap.set(
        precision,
        new Intl.NumberFormat('en-US', {
          minimumFractionDigits: precision,
          maximumFractionDigits: precision,
          useGrouping: false,
        }),
      );
    }
    return this.formattersMap.get(precision) as Intl.NumberFormat;
  }

  public formatNumberPrecision(num: number, precision: number): string {
    return this.getFormatter(precision).format(num);
  }
}

const formatterInstance = SingletonDecimalFormatter.getInstance();
export const formatNumberPrecision = (
  num: number,
  precision: number,
): string => {
  return formatterInstance.formatNumberPrecision(num, precision);
};
