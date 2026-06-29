import { KfCategoryTypes } from '@kungfu-tech/api/typings/enums';

export type SubMenuKeys = 'installed' | 'uninstalled';

export interface ExtSubMenusConfig {
  key: SubMenuKeys;
  name: string;
}

export type ExtSubMenusConfigs = Record<SubMenuKeys, ExtSubMenusConfig>;

export type AllExtCategoryTypes =
  | KfCategoryTypes
  | 'indexer'
  | 'matcher'
  | 'ui'
  | 'cli';

export interface ExtConfigForShow {
  id: string;
  key: string;
  name: string;
  category: AllExtCategoryTypes;
  categoryResolved: string;
  silent: boolean;
  needLogin: boolean;
  access: Record<string, string[]>;
  version: string;
  mainRepoVersion: string;
  description: string;
  readmePath: string;
  releaseNotePath: string;
  isPresetExt: boolean;
  ifCanUseInExtPage: boolean;
}

export type ExtConfigForShowList = ExtConfigForShow[];

export enum UnusableTypeEnum {
  NotSupport,
  NotLogin,
  NoAccess,
}
