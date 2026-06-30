import { KfCategoryEnum } from '@kungfu-tech/api/typings/enums';
import { KfCategory } from '@kungfu-tech/api/config/tradingConfig';
import VueI18n from '@kungfu-tech/api/language';
import { ExtSubMenusConfigs, AllExtCategoryTypes } from '../typings';

export const AuthingAccessKey = 'authing';

export const ExtManagerKeys = {
  ExtManagerMounted: 'manager:mounted',
  SelectExt: 'manager:select-ext',
};

const { t } = VueI18n.global;
export const extSubMenusConfigs: ExtSubMenusConfigs = {
  installed: {
    key: 'installed',
    name: t('extensionManager.installed'),
  },
  uninstalled: {
    key: 'uninstalled',
    name: t('extensionManager.uninstalled'),
  },
};

export const extCategoryData: Record<
  AllExtCategoryTypes,
  KungfuApi.KfTradeValueCommonData
> = {
  td: KfCategory[KfCategoryEnum.td],
  md: KfCategory[KfCategoryEnum.md],
  strategy: KfCategory[KfCategoryEnum.strategy],
  operator: KfCategory[KfCategoryEnum.operator],
  system: KfCategory[KfCategoryEnum.system],
  ui: {
    name: t('extensionManager.ui'),
    color: 'yellow',
    level: 110,
  },
  cli: {
    name: t('extensionManager.cli'),
    color: 'green',
    level: 40,
  },
  indexer: {
    name: t('extensionManager.indexer'),
    color: 'blue',
    level: 50,
  },
  matcher: {
    name: t('extensionManager.matcher'),
    color: 'purple',
    level: 50,
  },
};
