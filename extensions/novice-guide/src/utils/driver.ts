import { driver, Driver, Config, DriveStep } from 'driver.js';
import 'driver.js/dist/driver.css';

import VueI18n, { useLanguage } from '@kungfu-tech/api/language';
import { addStylesheetRules } from './index';
import { getNoviceCache, setNoviceCacheByKey } from '../actions';
import { driverStyleRules } from '../style';

const { t } = VueI18n.global;

const locals = {
  'zh-CN': {
    noviceGuide: {
      next: '下一步',
      previous: '上一步',
      done: '结束引导',
      clickHereNext: '点击这里，继续下一步。',
      wait: '请稍等',
      Td: '交易账户柜台',
      Md: '行情柜台',
      Strategy: '策略',
      addLocation: '添加{name}',
      addLocationDesc: '点击添加按钮，添加{name}。',
      selectLocation: '选择{name}',
      selectLocationDesc: {
        Td: '功夫对接了市面上常见的交易柜台，你可以选择其中一个进行添加。',
        Md: '功夫对接了市面上常见的行情柜台，允许添加多个行情柜台，在不同场景使用不同行情。\n功夫内置sim 行情柜台，可使用sim行情柜台进行功能试用、模拟交易。',
      },
      ensureLocation: '确定选择{name}',
      ensureLocationDesc: '点击确定按钮，开始添加{name}。',
      locationForm: '添加{name}',
      locationFormDesc: {
        Td: '填写账号信息，允许同时添加同柜台多个账户，以便于快速切换账号下单。',
        Strategy: '输入策略ID（自定义）、指定策略文件路径以完成策略添加。',
      },
      locationAddFinish: '添加{name}流程结束',
      showAddedLocation: '已添加的{name}',
      showAddedLocationDesc: {
        Td: '已添加的交易账户将显示在面板中，可通过进程按钮一键启停交易账户。\n此外，还可在右侧操作栏对账户进行编辑、删除、查看日志等操作。',
        Md: '已添加的行情源将显示在面板中，可通过进程按钮一键启停行情源。\n此外，还可在右侧操作栏中打开行情源进程的日志或修改行情源的配置。',
        Strategy:
          '已添加的策略将显示在面板中，可通过进程按钮一键启停策略。\n此外，还可在右侧操作栏对策略进行删除、查看日志等操作。功夫中内置代码编辑器，支持客户端内修改代码，代码将实时保存到本地。',
      },
      startLocation: '启动{name}进程',
    },
  },
  'en-US': {
    noviceGuide: {
      next: 'Next',
      previous: 'Previous',
      done: 'Done',
      clickHereNext: 'Click here to continue',
      wait: 'Please wait',
      Strategy: 'Strategy',
      Td: 'Td',
      Md: 'Md',
      addLocation: 'Add {name}',
      addLocationDesc: 'Click add button，add {name}',
      selectLocation: 'Select {name}',
      selectLocationDesc: {
        Td: 'Kungfu connects to common trading counters on the market, and you can choose one of them to add.',
        Md: 'Kungfu docks common market counters, allowing multiple market counters to be added to use different markets in different scenes. Kung Fu built-in sim market counter, you can use sim market counter function trial, simulation trading.',
      },
      ensureLocation: 'Confirm selected {name}',
      ensureLocationDesc: 'Click confirm button, start add {name}',
      locationForm: 'Add {name}',
      locationFormDesc: {
        Td: 'Fill in the account information, allowing to add multiple accounts at the same time, quickly switch accounts to place orders.',
        Strategy:
          'Enter the strategy ID (custom) and specify the policy file path to complete the policy addition. Kungfu has built-in strategy examples.',
      },
      locationAddFinish: 'Add {name} progress finished',
      showAddedLocation: 'The added {name}',
      showAddedLocationDesc: {
        Td: 'The added trading account will be displayed in the panel, and the trading account can be started and stopped with one click of the process button. In addition, you can also edit, delete, view logs and other operations on the account in the action bar on the right.',
        Md: 'The added information source will be displayed in the panel, and the information source can be started and stopped with one key through the process button. In addition, you can also open the log of the line source process or modify the configuration of the line source in the action bar on the right.',
        Strategy:
          'The added policies will be displayed in the panel, and the policies can be started and stopped with one click of the process button. You can also delete policies and view logs in the Actions column on the right. Kung Fu in the built-in code editor, support the client to modify the code, the code will be saved to the local in real time.',
      },
      startLocation: 'Start {name} process',
    },
  },
};

const { isLanguageKeyAvailable } = useLanguage();

if (!isLanguageKeyAvailable('noviceGuide.next')) {
  Object.keys(locals).forEach((lang) => {
    VueI18n.global.mergeLocaleMessage(lang, locals[lang]);
  });
}

export const mergeDriverStep = (
  driver: Driver,
  to: DriveStep,
  from?: DriveStep,
) => {
  if (!from) return to;

  const newStep: DriveStep = {};
  const allKeys = Object.keys({ ...to, ...from });

  for (const key of allKeys) {
    const toValue = to[key];
    const fromValue = from[key];
    const toType = typeof toValue;
    const fromType = typeof fromValue;
    if (toValue) {
      if (fromValue) {
        if (toType === fromType) {
          if (toType === 'object') {
            if (Array.isArray(toValue)) {
              newStep[key] = fromValue;
            } else {
              newStep[key] = mergeDriverStep(driver, toValue, fromValue);
            }
          } else if (toType === 'function') {
            const toFunc = toValue.bind(driver);
            newStep[key] = (...args) => {
              toFunc(...args);
              fromValue(...args);
            };
          }
        } else {
          newStep[key] = toValue;
        }
      } else {
        newStep[key] = toValue;
      }
    } else {
      newStep[key] = fromValue;
    }
  }

  return newStep;
};

export const waitElementMounted = (selector: string, timeout = 10000) => {
  return new Promise<Element>((resolve, reject) => {
    const timer = setTimeout(() => {
      reject(new Error('timeout'));
    }, timeout);

    const interval = setInterval(() => {
      const element = document.querySelector(selector);
      if (element) {
        resolve(element);
        clearInterval(interval);
        clearTimeout(timer);
      }
    }, 100);
  });
};

export const buildDriverStep = (
  driver: Driver,
  element: string,
  title: string,
  description?: string,
  extraConfig?: DriveStep,
) => {
  const step: DriveStep = {
    element,
    popover: {
      title,
      description: description || '',
      showButtons: ['next', 'close'],
    },
  };
  return mergeDriverStep(driver, step, extraConfig);
};

export const buildCenterModalStep = (
  driver: Driver,
  title: string,
  description?: string,
  extraConfig?: DriveStep,
) => {
  let popoverElement: HTMLElement;
  const step: DriveStep = {
    popover: {
      title,
      description: description || '',
      showButtons: ['next', 'close'],
      onPopoverRender: (popover) => {
        popover.wrapper.classList.add('kf-driver-modal');
        popoverElement = popover.wrapper;
      },
      onNextClick: () => {
        popoverElement?.classList.add('kf-driver-modal');
        driver.moveNext();
      },
    },
  };
  return mergeDriverStep(driver, step, extraConfig);
};

export const buildLoadingModalStep = (
  driver: Driver,
  title: string,
  description?: string,
  promise = () => Promise.resolve(),
  extraConfig?: DriveStep,
) => {
  const step = buildCenterModalStep(
    driver,
    title,
    description || t('noviceGuide.wait'),
  );

  if (step.popover) {
    step.popover.showButtons = ['close'];
  }

  step.onHighlighted = () => {
    promise().then(() => {
      driver.moveNext();
    });
  };

  return mergeDriverStep(driver, step, extraConfig);
};

export const buildWaitInputStep = (
  driver: Driver,
  element: string,
  title: string,
  description?: string,
  validate?: (value: string) => boolean,
  extraConfig?: DriveStep,
) => {
  const setNextState = (target: boolean) => {
    const state = driver.getState();
    const popover = state.popover;
    if (popover) {
      popover.nextButton.classList.toggle(
        'driver-popover-btn-disabled',
        !target,
      );
      popover.nextButton.disabled = !target;
    }
  };

  const inputHandler = (e) => {
    const event = e as InputEvent;
    const value = (event.target as HTMLInputElement).value || '';
    if (validate) {
      const isValid = validate(value);
      setNextState(isValid);
    } else {
      setNextState(!!value);
    }
  };

  const step: DriveStep = {
    element,
    popover: {
      title,
      description,
      showButtons: ['close', 'next'],
      disableButtons: ['next'],
      onNextClick: (element) => {
        if (element) {
          if (element instanceof HTMLInputElement) {
            element.removeEventListener('input', inputHandler);
          }
        }
        driver.moveNext();
      },
      onCloseClick: (element) => {
        if (element) {
          if (element instanceof HTMLInputElement) {
            element.removeEventListener('input', inputHandler);
          }
        }
        driver.destroy();
      },
    },
    onHighlighted: (element?: Element) => {
      if (element) {
        if (element instanceof HTMLInputElement) {
          element.addEventListener('input', inputHandler);
        }
      }
    },
  };

  return mergeDriverStep(driver, step, extraConfig);
};

export const buildWaitClickStep = (
  driver: Driver,
  element: string,
  title: string,
  description?: string,
  onClick?: () => void,
  extraConfig?: DriveStep,
) => {
  const clickHandler = () => {
    if (onClick) {
      onClick();
    } else {
      driver.moveNext();
    }
  };

  const step: DriveStep = {
    element,
    popover: {
      title,
      description: description || t('noviceGuide.clickHereNext'),
      showButtons: ['close'],
      onCloseClick: (element) => {
        if (element) {
          element.removeEventListener('click', clickHandler);
        }
        driver.destroy();
      },
    },
    onHighlighted: (element?: Element) => {
      if (element) {
        element.addEventListener('click', clickHandler, {
          once: true,
        });
      }
    },
  };

  return mergeDriverStep(driver, step, extraConfig);
};

export const buildAddLocationSteps = (
  driver: Driver,
  category: 'Td' | 'Md' | 'Strategy',
  showLast = true,
  start = true,
  finishCb?: () => void,
) => {
  const name = t(`noviceGuide.${category}`);
  const steps: DriveStep[] = [
    buildWaitClickStep(
      driver,
      `#${category} .kf-dashboard__header .header-actions > .kf-dashboard-item__warp:last-child button`,
      t('noviceGuide.addLocation', { name }),
      t('noviceGuide.addLocationDesc', { name }),
    ),
    ...(category !== 'Strategy'
      ? [
          buildDriverStep(
            driver,
            '.kf-set-source-modal .ant-modal-content .ant-modal-body',
            t('noviceGuide.selectLocation', { name }),
            t(`noviceGuide.selectLocationDesc.${category}`),
            {
              popover: {
                side: 'bottom',
              },
            },
          ),
        ]
      : []),
    ...(category !== 'Md'
      ? [
          ...(category !== 'Strategy'
            ? [
                buildWaitClickStep(
                  driver,
                  '.kf-set-source-modal .ant-modal-content .ant-modal-footer .ant-btn-primary',
                  t('noviceGuide.ensureLocation', { name }),
                  t('noviceGuide.ensureLocationDesc', { name }),
                ),
              ]
            : []),
          buildDriverStep(
            driver,
            '.kf-set-by-config-modal .ant-modal-content .ant-modal-body',
            t('noviceGuide.locationForm', { name }),
            t(`noviceGuide.locationFormDesc.${category}`),
            {
              popover: {
                side: 'bottom',
              },
            },
          ),
          buildWaitClickStep(
            driver,
            '.kf-set-by-config-modal .ant-modal-content .ant-modal-footer > button:nth-child(1)',
            t('noviceGuide.locationAddFinish', { name }),
          ),
        ]
      : [
          buildWaitClickStep(
            driver,
            '.kf-set-source-modal .ant-modal-content .ant-modal-footer > button:nth-child(1)',
            t('noviceGuide.locationAddFinish', { name }),
          ),
        ]),
    ...(showLast
      ? [
          buildDriverStep(
            driver,
            `#${category} .kf-dashboard__body .ant-table-body tbody > tr:nth-child(2)`,
            t('noviceGuide.showAddedLocation', { name }),
            t(`noviceGuide.showAddedLocationDesc.${category}`),
          ),
        ]
      : []),
    ...(start
      ? [
          buildWaitClickStep(
            driver,
            `#${category} .kf-dashboard__body .ant-table-body tbody tr:nth-child(2) button.ant-switch`,
            t('noviceGuide.startLocation', { name }),
            '',
            () => {
              if (finishCb) {
                finishCb();
              } else {
                driver.moveNext();
              }
            },
          ),
        ]
      : []),
  ];

  return steps;
};

export const generateDriverConfig = (): Config => ({
  showProgress: true,
  allowClose: false,
  allowKeyboardControl: false,
  disableActiveInteraction: false,
  stagePadding: 5,
  overlayOpacity: 0.6,
  popoverClass: 'kf-driver',
  progressText: '{{current}} / {{total}}',
  showButtons: ['next', 'close'],
  nextBtnText: t('noviceGuide.next'),
  doneBtnText: t('noviceGuide.done'),
});

export const ifTargetDriverHasRan = (id: string) => {
  return !!getNoviceCache()[id];
};

interface CustomDriver extends Driver {
  drive: (stepIndex?: number, forceRun?: boolean) => void;
  setHasRan: (hasRan?: boolean) => void;
  hasRan: boolean;
}
export const generateDriver = (
  id: string,
  config: Config,
  skipIfHasRan = true,
): CustomDriver => {
  if (!globalThis.driverCssAdded) {
    addStylesheetRules(driverStyleRules);
    globalThis.driverCssAdded = true;
  }

  const customDriver = driver(config) as CustomDriver;
  const drive = customDriver.drive.bind(customDriver);

  customDriver.hasRan = ifTargetDriverHasRan(id);
  customDriver.setHasRan = (hasRan = true) => {
    setNoviceCacheByKey(id, hasRan);
    customDriver.hasRan = hasRan;
  };

  customDriver.drive = (stepIndex?: number, forceRun = false) => {
    const defaultDrive = () => {
      customDriver.setHasRan(true);
      drive(stepIndex);
    };

    if (forceRun) {
      return defaultDrive();
    }

    if (skipIfHasRan) {
      if (customDriver.hasRan) return;

      return defaultDrive();
    }

    return defaultDrive();
  };

  return customDriver;
};
