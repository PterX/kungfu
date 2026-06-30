import { kfLogger } from '@kungfu-tech/api/utils/logUtils';

type Callback = (...args: unknown[]) => Promise<void>;
type ClearRegister = { clear: () => boolean };

type RegisterReturnType = ClearRegister | false;

export class ProcessActionHook {
  private callbacksMap: Record<string, Map<string, Callback>> = {};

  register(
    processId: string,
    actionKey: string,
    callback: Callback,
  ): RegisterReturnType {
    if (!this.callbacksMap[processId]) {
      this.callbacksMap[processId] = new Map();
    }

    // 覆盖或设置新的 callback
    this.callbacksMap[processId].set(actionKey, callback);
    kfLogger.info(
      `Registered a callback for processId: ${processId}, actionKey: ${actionKey}`,
    );

    return {
      clear: () => this.clear(processId, actionKey),
    };
  }

  async trigger(processId: string, actionKey: string) {
    const targetMap = this.callbacksMap[processId];
    if (!targetMap || !targetMap.has(actionKey)) {
      kfLogger.warn(
        `No callback found for processId: ${processId}, actionKey: ${actionKey}`,
      );
      return;
    }

    try {
      const callback = targetMap.get(actionKey);
      if (callback) {
        await callback();
      }
      kfLogger.info(
        `Successfully triggered callback for processId: ${processId}, actionKey: ${actionKey}`,
      );
    } catch (error) {
      kfLogger.error(
        `Error triggering callback for processId: ${processId}, actionKey: ${actionKey}, Error: \n${error}`,
      );
    }
  }

  clear(processId: string, actionKey: string): boolean {
    const targetMap = this.callbacksMap[processId];
    if (!targetMap || !targetMap.has(actionKey)) {
      kfLogger.warn(
        `No callback found to clear for processId: ${processId}, actionKey: ${actionKey}`,
      );
      return false;
    }
    targetMap.delete(actionKey);
    kfLogger.info(
      `Successfully cleared callback for processId: ${processId}, actionKey: ${actionKey}`,
    );
    return true;
  }

  clearAll(): boolean {
    this.callbacksMap = {};
    kfLogger.info('Successfully cleared all callbacks');
    return true;
  }
}
