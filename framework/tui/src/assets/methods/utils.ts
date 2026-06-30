import path from 'path';
import fse from 'fs-extra';
import inquirer from 'inquirer';
import colors from 'colors';
import checkboxPlusPrompt from 'inquirer-checkbox-plus-prompt';
import { KF_INSTRUMENTS_PATH } from '@kungfu-tech/api/config/pathConfig';
import { KfCategoryTypes } from '@kungfu-tech/api/typings/enums';
import resolveExtConfigHook from '@kungfu-tech/api/hooks/resolveExtConfigHook';
import { initFormStateByConfig } from '@kungfu-tech/api/utils/busiUtils';
import { ExchangeIds } from '@kungfu-tech/api/config/tradingConfig';
import {
  getKfCliExtensionConfig,
  getAvailCliExtServiceList,
} from '@kungfu-tech/api/utils/extUtils';
import {
  getIdByKfLocation,
  getProcessIdByKfLocation,
  loopToRunProcess,
  getPrimaryKeys,
} from '@kungfu-tech/api/utils/commonUtils';
import {
  replaceNonAlphaNumericWithSpace,
  getCombineValueByPrimaryKeys,
} from '@kungfu-tech/api/utils/busiUtils';
import { getAllKfConfigOriginData } from '@kungfu-tech/api/actions';
import {
  BrokerStateStatus,
  Pm2ProcessStatus,
} from '@kungfu-tech/api/config/tradingConfig';
import { PromptInputType, PromptQuestion } from '../../typings';
import { startExtService } from '@kungfu-tech/api/utils/processUtils';
import { Proc } from 'pm2';
import { globalState } from '../actions/globalState';
import { program } from 'commander';
import { SpecialWordsReg } from '@kungfu-tech/api/config/systemConfig';
import { PathPrompt } from 'inquirer-path-pro';
import VueI18n from '@kungfu-tech/api/language';
const { t } = VueI18n.global;

inquirer.registerPrompt('checkbox-plus', checkboxPlusPrompt);
inquirer.registerPrompt('path', PathPrompt);

export const parseToString = (
  targetList: (string | number)[],
  columnWidth: (string | number)[],
  pad = 2,
) => {
  return targetList
    .map((item: string | number, i: number) => {
      if (item + '' === '0') item = '0';
      item = (item || '').toString();

      const lw = item
        .replace(/\u001b\[1m/g, '')
        .replace(/\u001b\[22m/g, '')
        .replace(/\u001b\[31m/g, '')
        .replace(/\u001b\[32m/g, '')
        .replace(/\u001b\[33m/g, '')
        .replace(/\u001b\[34m/g, '')
        .replace(/\u001b\[35m/g, '')
        .replace(/\u001b\[36m/g, '')
        .replace(/\u001b\[37m/g, '')
        .replace(/\u001b\[38m/g, '')
        .replace(/\u001b\[39m/g, '')
        .replace(/\u001b\[45m/g, '')
        .replace(/\u001b\[49m/g, '');

      const len = lw.length;
      const colWidth: number | string = columnWidth[i] || 0;
      if (colWidth === 'auto') return item;
      const spaceLength = +colWidth - len;
      if (spaceLength < 0) return lw.slice(0, +colWidth);
      else if (spaceLength === 0) return item;
      else return item + new Array(spaceLength + 1).join(' ');
    })
    .join(new Array(pad + 2).join(' '));
};

export const getKfCategoryFromString = (
  typeString: string,
): KfCategoryTypes => {
  const isTd = typeString.toLocaleLowerCase().includes('td');
  const isMd = typeString.toLocaleLowerCase().includes('md');
  const isStrategy = typeString.toLocaleLowerCase().includes('strategy');
  const isOperator = typeString.toLocaleLowerCase().includes('operator');

  if (isTd) return 'td';
  else if (isMd) return 'md';
  else if (isStrategy) return 'strategy';
  else if (isOperator) return 'operator';
  else return 'system';
};

export const parseExtDataList = (
  extList: KungfuApi.KfAddableExtConfig[],
): string[] => {
  return extList.map((ext) => {
    const isArray = typeof ext.type === 'object';
    const type = isArray ? (ext.type || []).join(' ') : ext.type || '';
    return [ext.name, ext.key, type === 'unknown' ? '' : type].join('    ');
  });
};

export const getPromptQuestionsBySettings = async (
  data: {
    settings: KungfuApi.KfConfigItem[];
    primaryKeyAvoidRepeatCompareTarget?: string[];
    primaryKeyAvoidRepeatCompareExtra?: string;
    passPrimaryKeySpecialWordsVerify?: boolean;
  },
  initValue?: Record<string, KungfuApi.KfConfigValue>,
): Promise<KungfuApi.KfConfigValue> => {
  const {
    settings,
    primaryKeyAvoidRepeatCompareTarget,
    primaryKeyAvoidRepeatCompareExtra,
    passPrimaryKeySpecialWordsVerify,
  } = data;
  const formState = initFormStateByConfig(settings, initValue || {});
  const primaryKeys = getPrimaryKeys(settings);
  const questionsPromises = settings.map(
    async (item) =>
      await buildQuestionByKfConfigItem(
        item,
        item.type === 'password' ? '' : formState[item.key],
        !!initValue,
        async (value) => {
          if (!primaryKeys.includes(item.key)) {
            return true;
          }

          formState[item.key] = value;

          return primaryKeyValidator(value);
        },
      ),
  );

  const questions = await Promise.all(questionsPromises);

  function primaryKeyValidator(value: string): true | Error {
    const combineValue: string = getCombineValueByPrimaryKeys(
      primaryKeys,
      formState,
      primaryKeyAvoidRepeatCompareExtra || '',
    );

    if (!combineValue || replaceNonAlphaNumericWithSpace(value) === '') {
      return new Error(
        t('validate.single_characters', {
          value: combineValue,
        }),
      );
    }

    if (
      SpecialWordsReg.test(value || '') &&
      !passPrimaryKeySpecialWordsVerify
    ) {
      return new Error(t('validate.no_special_characters'));
    }

    if (
      (value || '').toString().includes('_') &&
      !passPrimaryKeySpecialWordsVerify
    ) {
      return new Error(t('validate.no_underline'));
    }

    if (
      (primaryKeyAvoidRepeatCompareTarget || [])
        .map((item): string => item.toLowerCase())
        .includes(combineValue.toLowerCase())
    ) {
      return initValue
        ? true
        : new Error(
            t('validate.value_existing', {
              value: combineValue,
            }),
          );
    }

    return true;
  }

  return inquirer
    .prompt(questions)
    .then((answers: Record<string, KungfuApi.KfConfigValue>) => {
      return trimAnswers(answers);
    });
};

export const getQuestionInputType = (
  originType: KungfuApi.KfConfigItemSupportedTypes,
): PromptInputType => {
  switch (originType) {
    case 'str':
    case 'password':
      return 'input';
    case 'int':
    case 'float':
      return 'number';
    case 'select':
    case 'radio':
    case 'side':
    case 'offset':
    case 'direction':
    case 'priceType':
    case 'hedgeFlag':
    case 'volumeCondition':
    case 'timeCondition':
    case 'commissionMode':
    case 'instrumentType':
    case 'td':
    case 'md':
    case 'strategy':
      return 'list';
    case 'bool':
      return 'confirm';
    case 'folder':
    case 'directory':
    case 'file':
      return 'path';
    case 'instrument':
      return 'autocomplete';
    case 'instruments':
    case 'multiSelect':
      return 'checkbox-plus';
    default:
      return 'input';
  }
};

export const renderSelect = (configItem: KungfuApi.KfConfigItem) => {
  if (configItem.type === 'select' || configItem.type === 'radio')
    return `(${(configItem.options || configItem.data || [])
      .map((item) => item.value || '')
      .join('|')})`;
  else return '';
};

const buildMessage = (
  configItem: KungfuApi.KfConfigItem,
  isUpdate: boolean,
  key: string,
): string => {
  const action = isUpdate ? 'Update' : 'Enter';
  const tip = configItem.tip ? `(${t(`${configItem.tip}`)})` : '';
  return `${action} ${key} ${renderSelect(configItem)} ${tip}`;
};

const getInstrumentChoicesAndMap = () => {
  const instrumentMap: Record<string, string> = {};
  if (!fse.pathExistsSync(KF_INSTRUMENTS_PATH)) {
    return { availableInstruments: [], instrumentMap };
  }
  try {
    const instruments = fse.readJSONSync(KF_INSTRUMENTS_PATH);
    if (!instruments) return { availableInstruments: [], instrumentMap };
    const availableInstruments = Object.keys(instruments).map((key) => {
      const item = instruments[key];
      instrumentMap[
        `${ExchangeIds[item.exchangeId].name} ${item.instrumentId} ${
          item.instrumentName
        }`
      ] = `${item.exchangeId}_${item.instrumentId}_${item.instrumentType}_${item.ukey}_${item.instrumentName}`;
      return `${ExchangeIds[item.exchangeId].name} ${item.instrumentId} ${
        item.instrumentName
      }`;
    });
    return { availableInstruments, instrumentMap };
  } catch (error) {
    return { availableInstruments: [], instrumentMap };
  }
};

export const buildQuestionByKfConfigItem = async (
  configItem: KungfuApi.KfConfigItem,
  defaultValue: KungfuApi.KfConfigValue | undefined,
  isUpdate = false,
  lastValidate?: (value: KungfuApi.KfConfigValue) => Promise<true | Error>,
) => {
  const { key, type } = configItem;
  const targetType = getQuestionInputType(type);
  const validateList: ((
    value: KungfuApi.KfConfigValue,
  ) => Promise<true | Error>)[] = [];

  validateList.push(async (value) => {
    if (configItem.required && value.toString() === '') {
      return new Error('Required');
    }

    if (configItem.primary) {
      if (SpecialWordsReg.test(value)) {
        return new Error(t('validate.no_special_characters'));
      }
    }

    if ((isUpdate && configItem.primary) || configItem.disabled) {
      if (value !== defaultValue) {
        return new Error(t('validate.default_value_tip'));
      }

      return true;
    }

    return true;
  });

  const baseQuestion: PromptQuestion = {
    type: targetType,
    name: key,
    message: buildMessage(configItem, isUpdate, key),
    validate: async (value: KungfuApi.KfConfigValue) => {
      for (const validate of validateList) {
        const res = await validate(value);
        if (res !== true) return res;
      }
      return true;
    },
    ...(targetType === 'path' ? { cwd: process.cwd().toString() } : {}),
    choices: (configItem.options || configItem.data || []).map(
      (item) => item.value,
    ),
    filter: (value: KungfuApi.KfConfigValue) =>
      targetType === 'number' && isNaN(value) ? 0 : value,
  };

  switch (type) {
    case 'multiSelect': {
      baseQuestion.pageSize = 10;
      baseQuestion.highlight = true;
      baseQuestion.searchable = true;
      baseQuestion.source = function (_, input = '') {
        return new Promise((resolve) => {
          const results = (configItem.options || configItem.data || [])
            .map((item) => item.value)
            .filter((item) =>
              item
                .toString()
                .toLocaleLowerCase()
                .includes(input.toLocaleLowerCase()),
            );
          resolve(results as string[]);
        });
      };
      break;
    }
    case 'instrument':
    case 'instruments': {
      const { availableInstruments, instrumentMap } =
        getInstrumentChoicesAndMap();

      baseQuestion.pageSize = 10;
      baseQuestion.highlight = true;
      baseQuestion.searchable = true;
      baseQuestion.message = `Select ${type}`;
      baseQuestion.source = function (_, input = '') {
        return new Promise((resolve) => {
          const results = availableInstruments.filter((item) =>
            item.toLocaleLowerCase().includes(input.toLocaleLowerCase()),
          );
          resolve(results);
        });
      };

      if (type === 'instrument') {
        baseQuestion.filter = (value: KungfuApi.KfConfigValue) => {
          if (!value) return defaultValue || value;
          return instrumentMap[value];
        };
      } else {
        baseQuestion.filter = (value: KungfuApi.KfConfigValue) => {
          if (!value.length) return defaultValue || value;
          return value.map((item) => instrumentMap[item]);
        };
      }
      break;
    }
    case 'percent': {
      validateList.push(async function (value) {
        const numValue = parseFloat(value);
        const isValid = numValue >= 0 && numValue <= 100;
        return (
          isValid || new Error('Please enter a valid number between 0 and 100.')
        );
      });
      baseQuestion.filter = function (value) {
        return `${parseFloat(value)}%`;
      };
      break;
    }
    case 'file': {
      validateList.push(async function (value) {
        const exists = await fse.pathExists(value);
        if (!exists) {
          return new Error(t('validate.file_path_not_exist'));
        }

        const stats = await fse.stat(value);
        const isFile = stats.isFile();
        return isFile || new Error(t('please_enter_file_path'));
      });
      break;
    }
    case 'directory':
    case 'folder': {
      validateList.push(async function (value) {
        const exists = await fse.pathExists(value);
        if (!exists) {
          return new Error(t('validate.file_path_not_exist'));
        }

        const stats = await fse.stat(value);
        const isDir = stats.isDirectory();
        return isDir || new Error(t('please_enter_folder_path'));
      });
      baseQuestion.directoryOnly = true;
      break;
    }
    case 'md': {
      const { md } = await getAllKfConfigOriginData();
      if (md) {
        baseQuestion.choices = md.map((item: KungfuApi.KfConfig) =>
          getIdByKfLocation(item),
        );
      }

      break;
    }
    default: {
      break;
    }
  }

  if (lastValidate) {
    validateList.push(lastValidate);
  }

  if (defaultValue) {
    baseQuestion.default = defaultValue;
  }
  return baseQuestion;
};

export const trimAnswers = (answers: Record<string, string | number>) => {
  Object.keys(answers || {}).forEach((key: string) => {
    if (typeof answers[key] === 'string') {
      answers[key] = (answers[key] as string).trim();
    }
  });
  return answers;
};

export const buildObjectFromKfConfigArray = (
  kfConfigs: KungfuApi.KfConfig[],
) => {
  return kfConfigs.reduce((data, item: KungfuApi.KfConfig) => {
    data[getIdByKfLocation(item)] = item;
    return data;
  }, {} as Record<string, KungfuApi.KfConfig>);
};

export const getKfLocation = (
  type: KfCategoryTypes,
  targetId: string,
): KungfuApi.KfLocation => {
  switch (type) {
    case 'md':
      return {
        category: 'md',
        group: targetId,
        name: targetId,
        mode: 'live',
      };
    case 'td':
      return {
        category: 'td',
        group: (targetId || '').parseSourceAccountId().source,
        name: (targetId || '').parseSourceAccountId().id,
        mode: 'live',
      };
    case 'operator':
      return {
        category: 'operator',
        group: 'default',
        name: targetId,
        mode: 'live',
      };
    case 'strategy':
      return {
        category: 'strategy',
        group: 'default',
        name: targetId,
        mode: 'live',
      };
    default:
      throw new Error(`Unsupported update category ${type}`);
  }
};

export const selectTargetKfConfig = async (
  noMd = false,
): Promise<KungfuApi.KfConfig | null> => {
  const { md, td, strategy, operator } = await getAllKfConfigOriginData();

  const mdTdStrategyOperatorList = [
    ...(noMd
      ? []
      : md.map((item) =>
          parseToString(
            [colors.yellow('md'), getIdByKfLocation(item)],
            [8, 'auto'],
            1,
          ),
        )),
    ...td.map((item) =>
      parseToString(
        [colors.cyan('td'), getIdByKfLocation(item)],
        [8, 'auto'],
        1,
      ),
    ),
    ...strategy.map((item) =>
      parseToString(
        [colors.blue('strategy'), getIdByKfLocation(item)],
        [8, 'auto'],
        1,
      ),
    ),
    ...operator.map((item) =>
      parseToString(
        [colors.green('operator'), getIdByKfLocation(item)],
        [8, 'auto'],
        1,
      ),
    ),
  ];

  const answers: { process: string } = await inquirer.prompt([
    {
      type: 'autocomplete',
      name: 'process',
      message: 'Select targeted md / td / strategy / operator  ',
      source: async (_answersSoFar: { process: string }, input: string) => {
        input = input || '';
        return mdTdStrategyOperatorList.filter((s: string): boolean =>
          s.includes(input),
        );
      },
    },
  ]);

  const processes = answers.process;

  const splits = processes.split(' ');
  const targetType = splits[0].trim();
  const targetId = splits[splits.length - 1].trim();
  const type = getKfCategoryFromString(targetType);
  const kfLocation = getKfLocation(type, targetId);
  const processId = getProcessIdByKfLocation(kfLocation);
  const searchList = [...md, ...td, ...strategy, ...operator];
  const targetIndex = searchList.findIndex((item: KungfuApi.KfConfig) => {
    const id = getProcessIdByKfLocation(item);
    if (id === processId) {
      return true;
    }

    return false;
  });

  if (targetIndex === -1) {
    return null;
  } else {
    return searchList[targetIndex];
  }
};

export const dealStatus = (status: string): string => {
  if (status === '--') return status;
  if (!Pm2ProcessStatus[status] && !BrokerStateStatus[status]) return status;
  const name: string =
    BrokerStateStatus[status]?.name || Pm2ProcessStatus[status]?.name || '';
  const level: number =
    BrokerStateStatus[status]?.level || Pm2ProcessStatus[status]?.level || 0;
  if (level >= 1) return colors.green(name);
  else if (level == 0) return colors.white(name);
  else if (level < 0) return colors.red(name);
  else return status;
};

export const dealMemory = (mem: number): string => {
  if (!mem) {
    return '--';
  }
  return Number((mem || 0) / (1024 * 1024)).kfToFixed(0) + 'MB';
};

export const dealProcessName = (name: string) => {
  return name
    ? name === 'archive'
      ? name.split('_').at(-1)
      : name.split('_').at(-2)
    : null;
};

export const calcHeaderWidth = (
  target: string[],
  wish: (string | number)[],
) => {
  wish = wish || [];
  return target.map((t: string, i) => {
    if (wish[i] === 'auto') return wish[i];
    if (t.length < (wish[i] || 0)) return wish[i];
    else return t.length;
  });
};

export const getCategoryName = (category: KfCategoryTypes) => {
  if (category === 'md') {
    return colors.yellow('Md');
  } else if (category === 'td') {
    return colors.cyan('Td');
  } else if (category === 'strategy') {
    return colors.blue('Strat');
  } else if (category === 'operator') {
    return colors.magenta('Oper');
  } else {
    return colors.bgMagenta('Sys');
  }
};

export const dealKfConfigValue = async (
  kfConfig: KungfuApi.KfConfig,
  extConfigs: KungfuApi.KfExtConfigs,
) => {
  const extConfig = await (
    globalThis.HookKeeper.getHooks()
      .resolveExtConfig as typeof resolveExtConfigHook
  ).trigger(kfConfig, extConfigs[kfConfig.category][kfConfig.group]);

  try {
    const settingsMap = extConfig?.settings?.reduce((pre, item) => {
      pre[item.key] = item.type;
      return pre;
    }, {});
    if (!settingsMap) return kfConfig.value;

    const kfConfigValue = JSON.parse(kfConfig.value);
    return JSON.stringify(
      Object.keys(kfConfigValue).reduce((pre, key) => {
        if (settingsMap[key] === 'password') {
          pre[key] = '******';
        }
        return pre;
      }, kfConfigValue),
    );
  } catch (error) {
    return kfConfig.value;
  }
};

export const colorNum = (num: number | string): string => {
  if (+num > 0) {
    return colors.red(num.toString());
  } else if (+num === 0) {
    return num.toString();
  } else {
    return colors.green(num.toString());
  }
};

export const getPadSpace = (num: number) => {
  return new Array(num + 1).join(' ');
};

export const isObject = (val) => {
  return Object.prototype.toString.call(val) === '[object Object]';
};

export const startAllExtServices = async () => {
  const availExtServices = await getAvailCliExtServiceList();
  return loopToRunProcess<void | Proc>(
    availExtServices.map((item) => {
      return () =>
        startExtService(item)
          .then((res) => {
            return res;
          })
          .catch((err) => console.error(err));
    }),
  );
};

interface KfExtModule {
  install: (gs: unknown, gt: typeof globalThis) => void;
}

const indexUse = (ext: KfExtModule) => {
  const { install } = ext;
  if (install) {
    install(program, globalThis);
  }
};

const dzxyUse = (ext: KfExtModule) => {
  const { install } = ext;
  if (install) {
    install(globalState, globalThis);
  }
};

export const useAllExtScript = () => {
  return getKfCliExtensionConfig()
    .then((allConfigs) =>
      Promise.all(
        Object.values(allConfigs).map((config) => {
          const { extPath, script } = config;
          const scriptPath = path.join(extPath, script);
          if (script && fse.pathExistsSync(scriptPath)) {
            return <Record<string, KfExtModule>>(
              __non_webpack_require__(scriptPath)
            );
          }

          return null;
        }),
      ),
    )
    .then((allExtModules) => {
      allExtModules.forEach((extModule) => {
        if (extModule) {
          dzxyUse(extModule['default']);
        }
      });
    });
};

export const useAllExtComponentByPosition = async (
  curPosition: 'index' | 'dzxy',
) => {
  const allConfigs = await getKfCliExtensionConfig();
  for (const config of Object.values(allConfigs)) {
    const { components, extPath } = config;
    if (components) {
      for (const key of Object.keys(components)) {
        const { entry, position } = components[key];

        if (position !== curPosition) continue;

        const componentPath = path.join(extPath, entry);
        if (entry && fse.pathExistsSync(componentPath)) {
          const extModule = <Record<string, KfExtModule>>(
            await __non_webpack_require__(componentPath)
          );
          if (extModule) {
            if (curPosition === 'index') {
              indexUse(extModule['default']);
            } else if (curPosition === 'dzxy') {
              dzxyUse(extModule['default']);
            }
          }
        }
      }
    }
  }
};
