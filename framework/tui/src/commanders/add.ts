import inquirer from 'inquirer';
import colors from 'colors';
import {
  getKfCategoryFromString,
  getPromptQuestionsBySettings,
  parseExtDataList,
} from '../assets/methods/utils';
import { KfCategoryTypes } from '@kungfu-tech/api/typings/enums';
import autocompletePrompt from 'inquirer-autocomplete-prompt';
import { getCombineValueByPrimaryKeys } from '@kungfu-tech/api/utils/busiUtils';
import {
  getExtConfigList,
  getKfExtensionConfig,
} from '@kungfu-tech/api/utils/extUtils';
import { PathPrompt } from 'inquirer-path-pro';
import {
  getPrimaryKeyFromKfConfigItem,
  getIdByKfLocation,
} from '@kungfu-tech/api/utils/commonUtils';
import { getAllKfConfigOriginData } from '@kungfu-tech/api/actions';
import { setKfConfig } from '@kungfu-tech/api/kungfu/store';
import { PromptAnswer } from '../typings';
import VueI18n from '@kungfu-tech/api/language';
const { t } = VueI18n.global;

inquirer.registerPrompt('autocomplete', autocompletePrompt);
inquirer.registerPrompt('path', PathPrompt);

export const selectMdTdStrategyOperator = async () => {
  const answers = await inquirer.prompt([
    {
      type: 'autocomplete',
      name: 'type',
      message: 'Select targeted md, td or strategy to add    ',
      source: async () => {
        return [
          colors.yellow('md'),
          colors.cyan('td'),
          colors.blue('strategy'),
          colors.green('operator'),
        ];
      },
    },
  ]);

  return getKfCategoryFromString(answers.type);
};

export const addMdTdStrategyOperator = async (
  type: KfCategoryTypes,
): Promise<boolean> => {
  const extConfigs = await getKfExtensionConfig();

  if (type === 'md') {
    const { md } = await getAllKfConfigOriginData();
    const extDataList = getExtConfigList(
      extConfigs,
      'md',
    ) as KungfuApi.KfMdExtConfig[];
    const extStrList = parseExtDataList(extDataList);
    const { source } = await selectKfExtPrompt(extStrList);
    const extKey = source.split('    ')[1];
    const extConfig =
      await globalThis.HookKeeper.getHooks().resolveExtConfig.trigger(
        {
          category: 'md',
          group: extKey,
          name: '*',
          mode: '*',
        },
        extConfigs['md'][extKey],
      );
    const settings = extConfig?.settings;

    if (settings === undefined) {
      throw new Error('Please check md extension config');
    }
    const mdIdList = md.map((item: KungfuApi.KfLocation): string =>
      getIdByKfLocation(item),
    );
    const primaryKeyAvoidRepeatCompareTarget = mdIdList;
    const primaryKeyAvoidRepeatCompareExtra = extKey;
    const passPrimaryKeySpecialWordsVerify = false;

    const formState = await getPromptQuestionsBySettings({
      settings,
      primaryKeyAvoidRepeatCompareTarget,
      primaryKeyAvoidRepeatCompareExtra,
      passPrimaryKeySpecialWordsVerify,
    });
    const kfLocation: KungfuApi.KfLocation = {
      category: 'md',
      group: extKey,
      name: extKey,
      mode: 'live',
    };

    return setKfConfig(
      kfLocation,
      JSON.stringify({
        ...formState,
        add_time: +new Date().getTime() * Math.pow(10, 6),
      }),
    );
  } else if (type === 'td') {
    const { td } = await getAllKfConfigOriginData();
    const extDataList = getExtConfigList(
      extConfigs,
      'td',
    ) as KungfuApi.KfTdExtConfig[];
    const extStrList = parseExtDataList(extDataList);
    const { source } = await selectKfExtPrompt(extStrList);
    const extKey = source.split('    ')[1];
    const extConfig =
      await globalThis.HookKeeper.getHooks().resolveExtConfig.trigger(
        {
          category: 'td',
          group: extKey,
          name: '*',
          mode: '*',
        },
        extConfigs['td'][extKey],
      );
    const settings = extConfig?.settings;

    if (settings === undefined) {
      throw new Error('Please check td extension config');
    }

    const TdIdList = td.map(
      (item: KungfuApi.KfLocation): string => `${item.group}_${item.name}`,
    );
    const primaryKeyAvoidRepeatCompareTarget = TdIdList;
    const primaryKeyAvoidRepeatCompareExtra = extKey;
    const passPrimaryKeySpecialWordsVerify = false;

    const formState = await getPromptQuestionsBySettings({
      settings,
      primaryKeyAvoidRepeatCompareTarget,
      primaryKeyAvoidRepeatCompareExtra,
      passPrimaryKeySpecialWordsVerify,
    });
    const primaryKeys = getPrimaryKeyFromKfConfigItem(settings).map(
      (item) => item.key,
    );
    const combinedValue = getCombineValueByPrimaryKeys(primaryKeys, formState);
    const kfLocation: KungfuApi.KfLocation = {
      category: 'td',
      group: extKey,
      name: combinedValue,
      mode: 'live',
    };

    return setKfConfig(
      kfLocation,
      JSON.stringify({
        ...formState,
        add_time: +new Date().getTime() * Math.pow(10, 6),
      }),
    );
  } else if (type === 'strategy') {
    const { strategy } = await getAllKfConfigOriginData();
    const setStrategyConfig: KungfuApi.KfStrategyExtConfig = {
      type: [],
      name: t('strategyConfig.strategy'),
      category: 'strategy',
      key: 'default',
      extPath: '',
      version: '',
      description: '',
      assets: {},
      dependencies: {},
      readmePath: '',
      releaseNotePath: '',
      silent: false,
      access: {},
      settings: [
        {
          key: 'strategy_id',
          name: t('strategyConfig.strategy_id'),
          type: 'str',
          primary: true,
          required: true,
          tip: t('strategyConfig.strategy_tip'),
        },
        {
          key: 'remarks',
          name: t('remarks'),
          type: 'str',
        },
        {
          key: 'file_path',
          name: t('strategyConfig.strategy_path'),
          type: 'file',
          tip: t('strategyConfig.strategy_path_tip'),
          required: true,
        },
      ],
    };
    const setStrategyConfigResolved: KungfuApi.KfStrategyExtConfig =
      await globalThis.HookKeeper.getHooks().resolveExtConfig.trigger(
        {
          category: 'strategy',
          group: 'default',
          name: '*',
          mode: '*',
        },
        setStrategyConfig,
      );

    const strategyIdList = strategy.map(
      (item: KungfuApi.KfLocation): string => item.name,
    );

    const primaryKeyAvoidRepeatCompareTarget = strategyIdList;

    const formState = await getPromptQuestionsBySettings({
      settings: setStrategyConfigResolved.settings,
      primaryKeyAvoidRepeatCompareTarget,
    });

    const primaryKeys = getPrimaryKeyFromKfConfigItem(
      setStrategyConfigResolved.settings,
    ).map((item) => item.key);
    const combinedValue = getCombineValueByPrimaryKeys(primaryKeys, formState);
    const kfLocation: KungfuApi.KfLocation = {
      category: 'strategy',
      group: 'default',
      name: combinedValue,
      mode: 'live',
    };

    return setKfConfig(
      kfLocation,
      JSON.stringify({
        ...formState,
        add_time: +new Date().getTime() * Math.pow(10, 6),
      }),
    );
  } else if (type === 'operator') {
    const { operator } = await getAllKfConfigOriginData();
    const fileOrext = await inquirer.prompt([
      {
        type: 'list',
        name: 'fileOrext',
        message: 'Select operator type    ',
        choices: ['file', 'extension'],
      },
    ]);
    if (fileOrext.fileOrext === 'file') {
      const operatorSettings: KungfuApi.KfConfigItem[] = [
        {
          key: 'operator_id',
          name: t('operatorConfig.operator_id'),
          type: 'str',
          primary: true,
          required: true,
          tip: t('operatorConfig.operator_id_tip'),
        },
        {
          key: 'remarks',
          name: t('remarks'),
          type: 'str',
        },
        {
          key: 'file_path',
          name: t('operatorConfig.file_path'),
          type: 'file',
          required: true,
        },
      ];

      const operatorList = operator.map((item: KungfuApi.KfLocation): string =>
        getIdByKfLocation(item),
      );

      const primaryKeyAvoidRepeatCompareTarget = operatorList;

      const formState = await getPromptQuestionsBySettings({
        settings: operatorSettings,
        primaryKeyAvoidRepeatCompareTarget,
      });
      const primaryKeys = getPrimaryKeyFromKfConfigItem(operatorSettings).map(
        (item) => item.key,
      );
      const combinedValue = getCombineValueByPrimaryKeys(
        primaryKeys,
        formState,
      );
      const kfLocation: KungfuApi.KfLocation = {
        category: 'operator',
        group: 'default',
        name: combinedValue,
        mode: 'live',
      };

      return setKfConfig(
        kfLocation,
        JSON.stringify({
          ...formState,
          add_time: +new Date().getTime() * Math.pow(10, 6),
        }),
      );
    } else {
      const extDataList = getExtConfigList(
        extConfigs,
        'operator',
      ) as KungfuApi.KfTdExtConfig[];
      const extStrList = parseExtDataList(extDataList);
      const { source } = await selectKfExtPrompt(extStrList);
      const extKey = source.split('    ')[1];
      const extConfig =
        await globalThis.HookKeeper.getHooks().resolveExtConfig.trigger(
          {
            category: 'operator',
            group: extKey,
            name: '*',
          },
          extConfigs['operator'][extKey],
        );
      const settings = extConfig?.settings;
      if (settings === undefined) {
        throw new Error('Please check operator extension config');
      }

      const operatorList = operator.map((item: KungfuApi.KfLocation): string =>
        getIdByKfLocation(item),
      );

      const primaryKeyAvoidRepeatCompareTarget = operatorList;
      const primaryKeyAvoidRepeatCompareExtra = extKey;

      const formState = await getPromptQuestionsBySettings({
        settings,
        primaryKeyAvoidRepeatCompareTarget,
        primaryKeyAvoidRepeatCompareExtra,
      });
      const primaryKeys = getPrimaryKeyFromKfConfigItem(settings).map(
        (item) => item.key,
      );
      const combinedValue = getCombineValueByPrimaryKeys(
        primaryKeys,
        formState,
      );
      const kfLocation: KungfuApi.KfLocation = {
        category: 'operator',
        group: extKey,
        name: combinedValue,
        mode: 'live',
      };

      return setKfConfig(
        kfLocation,
        JSON.stringify({
          ...formState,
          add_time: +new Date().getTime() * Math.pow(10, 6),
        }),
      );
    }
  }

  return Promise.resolve(false);
};

export const selectKfExtPrompt = async (
  extStrList: string[],
): Promise<PromptAnswer> => {
  return inquirer.prompt([
    {
      type: 'autocomplete',
      name: 'source',
      message: 'Select one type of source    ',
      source: async (_answersSoFar, input = '') => {
        return extStrList.filter((s: string) => s.includes(input));
      },
    },
  ]);
};
