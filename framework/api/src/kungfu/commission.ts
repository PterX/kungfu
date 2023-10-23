import { kfLogger, getResultUntilValuable } from '../utils/busiUtils';
import { commissionStore, longfist, promiseWithCachedPause } from './index';

export const getKfCommission = (
  watcher: KungfuApi.Watcher,
): Promise<KungfuApi.Commission[]> => {
  kfLogger.info('Get kungfu Commission');
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
  kfLogger.info('Set kungfu Commission');
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
