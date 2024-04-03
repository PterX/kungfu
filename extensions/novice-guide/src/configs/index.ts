import path from 'path';
import fse from 'fs-extra';

import { KfExtTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { getKfExtConfigList } from '@kungfu-trader/kungfu-js-api/utils/extUtils';
import {
  KF_CONFIG_DIR,
  KF_HOME,
} from '@kungfu-trader/kungfu-js-api/config/pathConfig';
import { addFileSync } from '@kungfu-trader/kungfu-js-api/utils/fileUtils';

import { AutoAddConfigs } from '../typings';

export const NG_CACHE_PATH = path.join(KF_CONFIG_DIR, 'noviceGuideCache.json');
if (!fse.pathExistsSync(NG_CACHE_PATH)) {
  addFileSync('', NG_CACHE_PATH, 'file', '{}');
}

export const STRATEGY_DEMO_DIR = path.join(KF_HOME, 'strategy_demo');
if (!fse.existsSync(STRATEGY_DEMO_DIR))
  addFileSync('', STRATEGY_DEMO_DIR, 'folder');

export const DefaultAutoAddConfigs: AutoAddConfigs = [
  {
    location: {
      category: 'td',
      group: 'sim',
      name: 'simTest',
      mode: 'live',
    },
    initValue: {
      account_id: 'simTest',
      account_name: '功夫示例',
      match_mode: 'fill',
    },
  },
  {
    location: {
      category: 'md',
      group: 'sim',
      name: 'sim',
      mode: 'live',
    },
  },
];

export const getStrategyDemoAutoAddConfigs = async () => {
  const configs: AutoAddConfigs = [];

  const extList = await getKfExtConfigList();
  const strategyDemoExts = extList.filter(
    (ext) => ext.type === KfExtTypeEnum.Example,
  );

  if (!strategyDemoExts.length) return configs;

  const strategyDemoAssets: Array<
    KungfuApi.KfExtOriginConfigAsset & { extPath: string; key: string }
  > = strategyDemoExts
    .map((ext) => {
      return Object.values(ext.assets).map((item) => ({
        ...item,
        extPath: ext.extPath,
        key: ext.key,
      }));
    })
    .flat()
    .sort((a, b) => {
      return (a.level || 0) - (b.level || 0);
    });

  strategyDemoAssets.forEach((asset) => {
    const strategyDemoPath = path.join(asset.extPath, asset.dest || '');
    if (!strategyDemoPath.endsWith('.py')) return;

    const targetStrategyPath = path.join(
      STRATEGY_DEMO_DIR,
      path.basename(strategyDemoPath),
    );
    if (fse.existsSync(strategyDemoPath) && !fse.existsSync(targetStrategyPath))
      fse.copySync(strategyDemoPath, targetStrategyPath);

    configs.push({
      location: {
        category: 'strategy',
        group: 'default',
        name: asset.key,
        mode: 'live',
      },
      initValue: {
        strategy_id: asset.key,
        remarks: asset.name || '功夫示例',
        file_path: targetStrategyPath,
        add_time: Date.now() * Math.pow(10, 6),
      },
    });
  });

  return configs;
};
