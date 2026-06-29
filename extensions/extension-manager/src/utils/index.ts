import { computed } from 'vue';
import { storeToRefs } from 'pinia';
import { KfCategoryTypes } from '@kungfu-tech/api/typings/enums';
import { getMainRepoVersionInDependencies } from '@kungfu-tech/api/utils/extUtils';
import VueI18n, { useLanguage } from '@kungfu-tech/api/language';
import { useExtManagerStore } from '../store';
import { extCategoryData } from '../configs';
import { ExtConfigForShow, ExtConfigForShowList } from '../typings';

const { t } = VueI18n.global;
const { isLanguageKeyAvailable } = useLanguage();

const CONNECTOR = '_';

const LOGIN_EXT_NAME = '@kungfu-tech/kfx-ui-login-authing';

export const buildExtId = ({
  key,
  category,
  name,
  isPresetExt,
}: {
  key: string;
  category: string;
  name: string;
  isPresetExt: boolean;
}) => [key, category, name, isPresetExt].join(CONNECTOR);

const translate = (key: string) => {
  if (isLanguageKeyAvailable(key)) {
    return t(key);
  }
  return key;
};

export const dealExtConfigForShow = (
  isPresetExt: boolean,
  config: KungfuApi.KfExtConfig,
): ExtConfigForShow | null => {
  if (!config) return null;
  const {
    key,
    name,
    category,
    silent,
    access,
    version,
    dependencies,
    description,
    readmePath,
  } = config;
  return {
    id: buildExtId({ ...config, isPresetExt }),
    key,
    name: translate(name),
    category: category as KfCategoryTypes,
    silent,
    access,
    categoryResolved: extCategoryData[category as KfCategoryTypes].name,
    version,
    mainRepoVersion: getMainRepoVersionInDependencies(dependencies),
    needLogin: !!dependencies[LOGIN_EXT_NAME],
    description,
    readmePath,
    releaseNotePath: '',
    isPresetExt,
    ifCanUseInExtPage: false,
  };
};

export const dealJsExtConfigForShow = (
  key: string,
  type: 'ui' | 'cli',
  isPresetExt: boolean,
  jsExtConfig: KungfuApi.KfUIExtConfig | KungfuApi.KfCliExtConfig,
): ExtConfigForShow | null => {
  if (!key || !jsExtConfig) return null;
  const {
    name,
    silent,
    access,
    version,
    dependencies,
    description,
    readmePath,
  } = jsExtConfig;
  const ifCanUseInExtPage =
    type === 'ui'
      ? (jsExtConfig as KungfuApi.KfUIExtConfig).position ===
        'extension_manager_use'
        ? true
        : false
      : false;
  return {
    id: buildExtId({
      key,
      category: type,
      name,
      isPresetExt,
    }),
    key,
    name: translate(name),
    category: type,
    categoryResolved: extCategoryData[type].name,
    silent,
    access,
    version,
    mainRepoVersion: getMainRepoVersionInDependencies(dependencies),
    needLogin: !!dependencies[LOGIN_EXT_NAME],
    description,
    readmePath,
    releaseNotePath: '',
    isPresetExt,
    ifCanUseInExtPage,
  };
};

export const getExtensionForShowByExtId = (
  extId: string,
  allExtensionForShows: ExtConfigForShowList,
): ExtConfigForShow | null => {
  if (!extId) return null;
  return allExtensionForShows.find((ext) => ext.id === extId) || null;
};

export const getExtensionForShowByExtKey = (
  extKey: string,
  allExtensionForShows: ExtConfigForShowList,
): ExtConfigForShow | null => {
  if (!extKey) return null;

  return allExtensionForShows.find((ext) => ext.key === extKey) || null;
};

export const useAllPresetExtension = () => {
  const extManagerStore = useExtManagerStore();
  const { allPresetExtensions } = storeToRefs(extManagerStore);

  const dealExtConfigsForShow = (
    extConfigs: KungfuApi.KfExtConfig[],
  ): ExtConfigForShowList => {
    return extConfigs
      .map((config) => {
        if (config.silent) return null;
        return dealExtConfigForShow(true, config);
      })
      .filter((item) => !!item) as ExtConfigForShowList;
  };

  const dealJsExtConfigsForShow = (
    type: 'ui' | 'cli',
    jsExtConfigs: KungfuApi.KfUIExtConfigs | KungfuApi.KfCliExtConfigs,
  ): ExtConfigForShowList => {
    return Object.keys(jsExtConfigs)
      .map((key) => {
        if (jsExtConfigs[key].silent) return null;

        if (jsExtConfigs[key].exhibit && !jsExtConfigs[key].components)
          return null;

        return dealJsExtConfigForShow(key, type, true, jsExtConfigs[key]);
      })
      .filter((item) => !!item) as ExtConfigForShowList;
  };

  const allPresetExtList = computed<ExtConfigForShowList>(() => {
    const { td, md, strategy, operator, system, ui, cli } =
      allPresetExtensions.value;

    const extConfigsResolved = dealExtConfigsForShow([
      ...Object.values(td || {}),
      ...Object.values(md || {}),
      ...Object.values(strategy || {}),
      ...Object.values(operator || {}),
      ...(Object.values(system || {})
        .map((item) => Object.values(item))
        .flat() as KungfuApi.KfSystemExtConfig[]),
    ] as KungfuApi.KfExtConfig[]);
    const uiExtConfigsResolved = dealJsExtConfigsForShow('ui', ui || {});
    const cliExtConfigsResolved = dealJsExtConfigsForShow('cli', cli || {});
    return [
      ...extConfigsResolved,
      ...uiExtConfigsResolved,
      ...cliExtConfigsResolved,
    ].filter((item) => !(item.key === 'extensionManager'));
  });

  const allPresetExtMap = computed(() => {
    const { td, md, strategy, operator, system, ui, cli } =
      allPresetExtensions.value;
    return {
      td: Object.values(td || {}),
      md: Object.values(md || {}),
      strategy: Object.values(strategy || {}),
      operator: Object.values(operator || {}),
      system: Object.values(system || {})
        .map((item) => Object.values(item))
        .flat() as KungfuApi.KfSystemExtConfig[],
      ui: Object.values(ui || {}),
      cli: Object.values(cli || {}),
    };
  });

  return {
    allPresetExtList,
    allPresetExtMap,
  };
};
