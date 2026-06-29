import { defineStore } from 'pinia';
import { getAllExtensions } from '@kungfu-tech/api/utils/extUtils';
import { ExtConfigForShow } from '../typings/index';

interface ManageStore {
  allPresetExtensions: KungfuApi.KfAllExtConfigs;
  currentExtension: ExtConfigForShow | null;
  isInExtUse: boolean;
}

const useExtManagerStore = defineStore('extensionManager', {
  state: () =>
    ({
      currentExtension: null,
      isInExtUse: false,
      allPresetExtensions: {},
    } as ManageStore),
  getters: {},
  actions: {
    setCurrentExtension(extension: ExtConfigForShow | null) {
      this.currentExtension = extension;
    },
    setIsInExtUse(target: boolean) {
      this.isInExtUse = target;
    },
    setAllExtensions() {
      return getAllExtensions().then((allExtensions) => {
        this.allPresetExtensions = allExtensions;
      });
    },
  },
});

export { useExtManagerStore };
