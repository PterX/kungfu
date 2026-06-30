import { getAllKfConfigOriginData } from '@kungfu-tech/api/actions';
import {
  buildObjectFromKfConfigArray,
  getPromptQuestionsBySettings,
  selectTargetKfConfig,
} from '../assets/methods/utils';
import { getKfExtensionConfig } from '@kungfu-tech/api/utils/extUtils';
import { getIdByKfLocation } from '@kungfu-tech/api/utils/commonUtils';
import { setKfConfig } from '@kungfu-tech/api/kungfu/store';
import VueI18n from '@kungfu-tech/api/language';
const { t } = VueI18n.global;

export const updateMdTdStrategy = async () => {
  const kfLocation = await selectTargetKfConfig();

  if (!kfLocation) {
    throw new Error('target is illegal kfLocation');
  }

  const extConfigs = await getKfExtensionConfig();
  const { md, td, strategy, operator } = await getAllKfConfigOriginData();

  if (kfLocation.category === 'md') {
    const targetMd = getTargetKfConfig(md, kfLocation);
    const initValue = JSON.parse(targetMd?.value || '{}');
    const extConfig =
      await globalThis.HookKeeper.getHooks().resolveExtConfig.trigger(
        {
          category: 'md',
          group: kfLocation.group,
          name: '*',
          mode: '*',
        },
        extConfigs['md'][kfLocation.group],
      );
    const settings = extConfig?.settings;

    if (settings === undefined) {
      throw new Error('Please check md extension config');
    }

    return buildPromptAndSetConfig(settings, initValue, kfLocation);
  } else if (kfLocation.category === 'td') {
    const targetTd = getTargetKfConfig(td, kfLocation);

    if (!targetTd) {
      throw new Error('targetTd is null');
    }

    const initValue = JSON.parse(targetTd.value || '{}');
    const extConfig =
      await await globalThis.HookKeeper.getHooks().resolveExtConfig.trigger(
        {
          category: 'td',
          group: kfLocation.group,
          name: '*',
          mode: '*',
        },
        extConfigs['td'][kfLocation.group],
      );
    const settings = extConfig?.settings;

    if (settings === undefined) {
      throw new Error('Please check td extension config');
    }

    return buildPromptAndSetConfig(settings, initValue, kfLocation);
  } else if (kfLocation.category === 'strategy') {
    const targetStrategy = getTargetKfConfig(strategy, kfLocation);

    if (!targetStrategy) {
      throw new Error('targetTd is null');
    }

    const initValue = JSON.parse(targetStrategy.value || '{}');
    const strategySettings: KungfuApi.KfConfigItem[] = [
      {
        key: 'strategy_id',
        name: t('strategyConfig.strategy_id'),
        type: 'str',
        primary: true,
        required: true,
        tip: t('strategyConfig.strategy_id_tip'),
      },
      {
        key: 'file_path',
        name: t('strategyConfig.file_path'),
        type: 'file',
        required: true,
      },
    ];

    return buildPromptAndSetConfig(strategySettings, initValue, kfLocation);
  } else if (kfLocation.category === 'operator') {
    const targetOperator = getTargetKfConfig(operator, kfLocation);
    let operatorSettings: KungfuApi.KfConfigItem[] = [];

    if (!targetOperator) {
      throw new Error('targetTd is null');
    }

    const initValue = JSON.parse(targetOperator.value || '{}');
    if (kfLocation.group === 'default') {
      operatorSettings = [
        {
          key: 'operator_id',
          name: t('operatorConfig.operator_id'),
          type: 'str',
          primary: true,
          required: true,
          tip: t('operatorConfig.operator_id_tip'),
        },
        {
          key: 'file_path',
          name: t('operatorConfig.file_path'),
          type: 'file',
          required: true,
        },
      ];
    } else {
      const extConfig =
        await globalThis.HookKeeper.getHooks().resolveExtConfig.trigger(
          {
            category: 'operator',
            group: kfLocation.group,
            name: '*',
          },
          extConfigs['operator'][kfLocation.group],
        );
      operatorSettings = extConfig?.settings;
    }

    return buildPromptAndSetConfig(operatorSettings, initValue, kfLocation);
  }

  return false;
};

function getTargetKfConfig(
  kfConfigs: KungfuApi.KfConfig[],
  kfLocation: KungfuApi.KfLocation,
): KungfuApi.KfConfig | null {
  const kfConfigMap = buildObjectFromKfConfigArray(kfConfigs);
  return kfConfigMap[getIdByKfLocation(kfLocation)] || null;
}

async function buildPromptAndSetConfig(
  settings: KungfuApi.KfConfigItem[],
  initValue: Record<string, KungfuApi.KfConfigValue>,
  kfLocation: KungfuApi.KfLocation,
): Promise<boolean> {
  let formState: KungfuApi.KfConfigValue = {};
  const categoryList = ['md', 'td', 'strategy', 'operator', 'system'];
  const category = kfLocation.category;
  if (categoryList.includes(category)) {
    const configData = await getAllKfConfigOriginData();
    const categoryType = configData[category];
    const idList = categoryType
      ? categoryType.map((item: KungfuApi.KfLocation): string =>
          getIdByKfLocation(item),
        )
      : [];
    const primaryKeyAvoidRepeatCompareTarget = idList;
    const primaryKeyAvoidRepeatCompareExtra = kfLocation.group;

    formState = await getPromptQuestionsBySettings(
      {
        settings,
        primaryKeyAvoidRepeatCompareTarget,
        primaryKeyAvoidRepeatCompareExtra,
      },
      initValue,
    );
  } else {
    formState = await getPromptQuestionsBySettings(
      {
        settings,
      },
      initValue,
    );
  }

  return setKfConfig(
    kfLocation,
    JSON.stringify({
      ...formState,
      add_time: +new Date().getTime() * Math.pow(10, 6),
    }),
  );
}
