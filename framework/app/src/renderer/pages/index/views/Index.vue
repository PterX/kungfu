<template>
  <div class="kf-index__warp">
    <KfRowColIter
      :board-id="0"
      :closable="true"
      :init-boards-map="curBoardsMap"
      :current-boards-store-id="'main'"
      :default-boards-map="curDefaultBoardsMap"
    ></KfRowColIter>
    <KfAddBoardModalVue
      v-if="addBoardModalVisible"
      v-model:visible="addBoardModalVisible"
      :target-board-id="addBoardTargetBoardId"
    ></KfAddBoardModalVue>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, onActivated, onDeactivated } from 'vue';

import KfRowColIter from '@kungfu-trader/kungfu-app/src/renderer/components/layout/KfRowColIter.vue';

import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';
import { defaultBoardsMap } from '@kungfu-trader/kungfu-app/src/renderer/assets/configs';
import KfAddBoardModalVue from '../../../components/public/KfAddBoardModal.vue';
import globalBus from '@kungfu-trader/kungfu-js-api/utils/globalBus';
import { Subscription } from 'rxjs';
import { useBoards } from '../store/board';
import { deepClone } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';

export default defineComponent({
  name: 'Index',

  components: {
    KfRowColIter,
    KfAddBoardModalVue,
  },

  setup() {
    const { setCurrentGlobalKfLocation, setDefaultCurrentGlobalKfLocation } =
      useGlobalStore();

    const dealDefaultBoardsHook =
      globalThis.HookKeeper.getHooks().dealBoardsMap;

    const addBoardModalVisible = ref<boolean>(false);
    const addBoardTargetBoardId = ref<number>(-1);

    let subscription: Subscription;
    const { getLocalBoardsMap } = useBoards();
    const curDefaultBoardsMap = dealDefaultBoardsHook.trigger(
      defaultBoardsMap,
    ) as KfLayout.BoardsMap;
    const curBoardsMap: KfLayout.BoardsMap =
      getLocalBoardsMap('main') || deepClone(defaultBoardsMap);

    onActivated(() => {
      subscription = globalBus.subscribe((data: KfEvent.KfBusEvent) => {
        if (data.tag === 'addBoard') {
          addBoardModalVisible.value = true;
          addBoardTargetBoardId.value = data.boardId;
        }

        if (data.tag === 'main') {
          if (data.name == 'record-before-quit') {
            window.watcher && window.watcher.quit();
          }
        }
      });

      setCurrentGlobalKfLocation(null);
      setDefaultCurrentGlobalKfLocation();
    });

    onDeactivated(() => {
      subscription.unsubscribe();
    });

    return {
      curBoardsMap,
      curDefaultBoardsMap,
      addBoardModalVisible,
      addBoardTargetBoardId,
    };
  },
});
</script>

<style lang="less">
.kf-index__warp {
  height: 100%;
  width: 100%;

  & > .kf-drag-row__warp {
    height: 100%;
  }
}
</style>
