import {
  getCurrentInstance,
  onBeforeUnmount,
  onDeactivated,
  onMounted,
  ref,
  Ref,
} from 'vue';
import path from 'path';
import {
  useExtConfigsRelated,
  useProcessStatusDetailData,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { kfConfigItemsToProcessArgs } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import globalBus from '@kungfu-trader/kungfu-js-api/utils/globalBus';
import { startTradingTask } from '@kungfu-trader/kungfu-js-api/actions/tradingTask';
import VueI18n, {
  useLanguage,
} from '@kungfu-trader/kungfu-js-api/language/index';
import { messagePrompt } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
const { t } = VueI18n.global;

export const useTradingTask = (): {
  setTradingTaskModalVisible: Ref<boolean>;
  currentSelectedTradingTaskExtKey: Ref<string>;
  setTradingTaskConfigPayload: Ref<KungfuApi.SetKfConfigPayload>;
  triggerAddTradingTask: (
    extKey: string,
    payload: KungfuApi.SetKfConfigPayload,
  ) => void;
  handleOpenSetTradingTaskModal: (
    type: KungfuApi.ModalChangeType,
    selectedExtKey: string,
    taskInitValue?: Record<string, KungfuApi.KfConfigValue>,
  ) => void;
  handleConfirmAddUpdateTask: (
    data: {
      formState: Record<string, KungfuApi.KfConfigValue>;
      idByPrimaryKeys: string;
      configSettings: KungfuApi.KfConfigItem[];
      changeType: KungfuApi.ModalChangeType;
    },
    extKey: string,
  ) => void;
} => {
  const setTradingTaskModalVisible = ref<boolean>(false);
  const currentSelectedTradingTaskExtKey = ref<string>('');
  const setTradingTaskConfigPayload = ref<KungfuApi.SetKfConfigPayload>({
    type: 'add',
    title: t('tradingTaskConfig.tradingTask'),
    config: {} as KungfuApi.KfStrategyExtConfig,
  });
  const { extConfigs } = useExtConfigsRelated();
  const { processStatusData } = useProcessStatusDetailData();
  const { isLanguageKeyAvailable } = useLanguage();
  const app = getCurrentInstance();
  const tradingTaskCategory = 'strategy';
  const { success, error } = messagePrompt();

  const handleOpenSetTradingTaskModal = (
    type = 'add' as KungfuApi.ModalChangeType,
    selectedExtKey: string,
    taskInitValue?: Record<string, KungfuApi.KfConfigValue>,
  ) => {
    if (selectedExtKey === '') {
      error(t('tradingTaskConfig.key_inexistence'));
      return;
    }

    const extConfig: KungfuApi.KfStrategyExtConfig = (extConfigs.value[
      tradingTaskCategory
    ] || {})[selectedExtKey];

    if (!extConfig) {
      error(
        t('tradinTaskConfig.plugin_inexistence', {
          key: selectedExtKey,
        }),
      );
      return;
    }

    currentSelectedTradingTaskExtKey.value = selectedExtKey;
    setTradingTaskConfigPayload.value.type = type;
    setTradingTaskConfigPayload.value.title = `${
      isLanguageKeyAvailable(extConfig.name)
        ? t(extConfig.name)
        : extConfig.name
    }`;
    setTradingTaskConfigPayload.value.config = extConfig;
    setTradingTaskConfigPayload.value.initValue = undefined;

    if (taskInitValue) {
      setTradingTaskConfigPayload.value.initValue = taskInitValue;
    }

    if (!extConfig?.settings?.length) {
      error(t('tradingTaskConfig.configuration_inexistence'));
      return;
    }

    triggerAddTradingTask(selectedExtKey, setTradingTaskConfigPayload.value);
  };

  const triggerAddTradingTask = (
    extKey: string,
    payload: KungfuApi.SetKfConfigPayload,
  ) => {
    if (app?.proxy) {
      app?.proxy.$globalBus.next({
        tag: 'setTradingTask',
        extKey,
        payload,
      });
    }
  };

  const handleConfirmAddUpdateTask = (
    data: {
      formState: Record<string, KungfuApi.KfConfigValue>;
      configSettings: KungfuApi.KfConfigItem[];
      idByPrimaryKeys: string;
      changeType: KungfuApi.ModalChangeType;
    },
    extKey: string,
  ) => {
    const { formState } = data;
    const taskLocation: KungfuApi.KfLocation = {
      category: 'strategy',
      group: extKey,
      name: new Date().getTime().toString(),
      mode: 'live',
    };

    const extConfig: KungfuApi.KfStrategyExtConfig = (extConfigs.value[
      'strategy'
    ] || {})[extKey];

    if (!extConfig) {
      error(
        t('tradinTaskConfig.plugin_inexistence', {
          key: extKey,
        }),
      );
      return;
    }

    if (!extConfig.extPath) {
      error(t('tradingTaskConfig.configuration_inexistence'));
      return;
    }

    const args: string = kfConfigItemsToProcessArgs(
      data.configSettings || extConfig.settings || [],
      formState,
    );

    const soPath = path.join(extConfig.extPath, extKey);
    return startTradingTask(
      window.watcher,
      taskLocation,
      processStatusData.value,
      soPath,
      args,
      data.configSettings,
    )
      .then(() => {
        success();
      })
      .catch((err: Error) => error(err.message || t('operation_failed')));
  };

  onMounted(() => {
    if (app?.proxy) {
      const subscription = globalBus.subscribe((data: KfEvent.KfBusEvent) => {
        if (data.tag === 'setTradingTask') {
          currentSelectedTradingTaskExtKey.value = data.extKey;
          setTradingTaskConfigPayload.value = data.payload;
          setTradingTaskModalVisible.value = true;
        }
      });

      onBeforeUnmount(() => {
        subscription.unsubscribe();
      });

      onDeactivated(() => {
        subscription.unsubscribe();
      });
    }
  });

  return {
    setTradingTaskModalVisible,
    currentSelectedTradingTaskExtKey,
    setTradingTaskConfigPayload,
    triggerAddTradingTask,
    handleOpenSetTradingTaskModal,
    handleConfirmAddUpdateTask,
  };
};
