import inquirer from 'inquirer';
import colors from 'colors';
import {
  getKfCategoryFromString,
  getPromptQuestionsBySettings,
  parseExtDataList,
} from '../assets/methods/utils';
import { KfCategoryTypes } from '@kungfu-trader/kungfu-js-api/typings/enums';
import autocompletePrompt from 'inquirer-autocomplete-prompt';
import { PathPrompt } from 'inquirer-path';
import {
  getCombineValueByPrimaryKeys,
  getExtConfigList,
  getKfExtensionConfig,
  getPrimaryKeyFromKfConfigItem,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { setKfConfig } from '@kungfu-trader/kungfu-js-api/kungfu/store';
import { PromptAnswer } from '../typings';

inquirer.registerPrompt('autocomplete', autocompletePrompt);
inquirer.registerPrompt('path', PathPrompt);

export const selectMdTdStrategy = async () => {
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
        },
        extConfigs['md'][extKey],
      );
    const settings = extConfig?.settings;

    if (settings === undefined) {
      throw new Error('Please check md extension config');
    }

    const formState = await getPromptQuestionsBySettings(settings);
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
        },
        extConfigs['td'][extKey],
      );
    const settings = extConfig?.settings;

    if (settings === undefined) {
      throw new Error('Please check td extension config');
    }

    const formState = await getPromptQuestionsBySettings(settings);
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
    const strategySettings: KungfuApi.KfConfigItem[] = [
      {
        key: 'strategy_id',
        name: '策略ID',
        type: 'str',
        primary: true,
        required: true,
        tip: '需保证该策略ID唯一',
      },
      {
        key: 'file_path',
        name: '策略路径',
        type: 'file',
        required: true,
      },
    ];

    const formState = await getPromptQuestionsBySettings(strategySettings);
    const primaryKeys = getPrimaryKeyFromKfConfigItem(strategySettings).map(
      (item) => item.key,
    );
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
          name: '算子ID',
          type: 'str',
          primary: true,
          required: true,
          tip: '需保证该算子ID唯一',
        },
        {
          key: 'file_path',
          name: '文件路径',
          type: 'file',
          required: true,
        },
      ];

      const formState = await getPromptQuestionsBySettings(operatorSettings);
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

      const formState = await getPromptQuestionsBySettings(settings);
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
