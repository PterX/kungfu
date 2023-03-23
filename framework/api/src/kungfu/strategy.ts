import { setKfConfig } from './store';
import { getStrategyKfLocation, getKfConfig } from './store';
export const getStrategyById = (
  strategyId: string,
): Promise<Array<Code.Icodeinfo>> => {
  return new Promise((resolve, reject) => {
    const strategyData: KungfuApi.KfConfig | false = getKfConfig(strategyId);
    if (!strategyData) {
      reject(new Error('Failed to get strategy'));
      return;
    }

    const strategy: Array<Code.Icodeinfo> = [
      { ...JSON.parse(strategyData.value || '{}') },
    ];
    resolve(strategy);
  });
};

export const updateStrategyPath = async (
  strategyId: string,
  strategyPath: string,
) => {
  let addTime = +new Date().getTime() * Math.pow(10, 6);
  const strategyOld: Array<Code.Icodeinfo> = await getStrategyById(strategyId);
  const kfLocation = getStrategyKfLocation(strategyId);
  if (strategyOld.length) {
    addTime = strategyOld[0].add_time;
  }
  return new Promise((resolve) => {
    const strategy = setKfConfig(
      kfLocation,
      JSON.stringify({
        strategy_id: strategyId,
        file_path: strategyPath,
        add_time: addTime,
      }),
    );
    resolve(strategy);
  });
};
