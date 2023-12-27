import { getResultUntilValuable } from '../utils/commonUtils';
import { promiseWithCachedPause } from '../utils/tradingUtils';
import { commissionStore, longfist } from './index';

export const getKfCommission = (
  watcher: KungfuApi.Watcher,
): Promise<KungfuApi.Commission[]> => {
  return promiseWithCachedPause(watcher, () =>
    getResultUntilValuable(() => commissionStore.getAllCommission()).then(
      (allCommissions) =>
        allCommissions.sort((a, b) =>
          a.exchange_id.localeCompare(b.exchange_id),
        ),
    ),
  );
};

export const setKfCommission = (
  watcher: KungfuApi.Watcher,
  commissions: KungfuApi.Commission[],
): Promise<boolean> => {
  const kfCommissionData = longfist.types.Commission();
  const comissionsResolved = commissions
    .filter((item) => {
      return item.product_id && item.exchange_id;
    })
    .map((item: KungfuApi.Commission) => {
      return {
        ...kfCommissionData,
        ...item,
      };
    });

  return promiseWithCachedPause(watcher, () => {
    return getResultUntilValuable(() =>
      commissionStore.setAllCommission(comissionsResolved),
    );
  });
};
