import { setKfConfig } from './store';
import { getStrategyKfLocation, getKfConfig } from './store';
export const getCodeInfoById = (
  strategyId: string,
): Promise<Array<Code.Icodeinfo>> => {
  console.log(strategyId, 'strategyIdstrategyIdstrategyIdstrategyId');
  return new Promise((resolve, reject) => {
    const strategyData: KungfuApi.KfConfig | false = getKfConfig(strategyId);
    console.log(strategyData, 'strategyDatastrategyDatastrategyData');

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

export const updateCurrentCodePath = async (
  codeId: string,
  fileNewPath: string,
) => {
  let addTime = +new Date().getTime() * Math.pow(10, 6);
  const strategyOld: Array<Code.Icodeinfo> = await getCodeInfoById(codeId);
  const kfLocation = getStrategyKfLocation(codeId);
  if (strategyOld.length) {
    addTime = strategyOld[0].add_time;
  }
  return new Promise((resolve) => {
    const strategy = setKfConfig(
      kfLocation,
      JSON.stringify({
        code_id: codeId,
        file_path: fileNewPath,
        add_time: addTime,
      }),
    );
    resolve(strategy);
  });
};
