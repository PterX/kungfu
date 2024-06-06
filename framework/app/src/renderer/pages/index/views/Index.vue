<template>
  <div class="kf-index__warp">
    <KfBoards
      boards-id="main"
      :closable="true"
      :boards-map-builder="buildDefaultBoardsMap"
    ></KfBoards>
    <KfAddBoardModalVue
      v-if="addBoardModalVisible"
      v-model:visible="addBoardModalVisible"
      :target-board-id="addBoardTargetBoardId"
    ></KfAddBoardModalVue>
  </div>
</template>

<script lang="ts">
import { defineComponent, ref, onActivated, onDeactivated } from 'vue';
import { Subscription } from 'rxjs';

import KfBoards from '@kungfu-trader/kungfu-app/src/renderer/components/layout/KfBoards.vue';
import KfAddBoardModalVue from '../../../components/public/KfAddBoardModal.vue';

import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';
import { defaultBoardsMap } from '@kungfu-trader/kungfu-app/src/renderer/assets/configs';
import globalBus from '@kungfu-trader/kungfu-js-api/utils/globalBus';
import { deepClone } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';

export default defineComponent({
  name: 'Index',

  components: {
    KfBoards,
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
    const curDefaultBoardsMap = dealDefaultBoardsHook.trigger(
      defaultBoardsMap,
    ) as KfLayout.BoardsMap;
    const curBoardsMap: KfLayout.BoardsMap = deepClone(curDefaultBoardsMap);

    onActivated(() => {
      subscription = globalBus.subscribe((data: KfEvent.KfBusEvent) => {
        if (data.tag === 'addBoard') {
          addBoardModalVisible.value = true;
          addBoardTargetBoardId.value = data.boardId;
        }
      });

      setCurrentGlobalKfLocation(null);
      setDefaultCurrentGlobalKfLocation();
    });

    onDeactivated(() => {
      subscription.unsubscribe();
    });

    return {
      buildDefaultBoardsMap: () => curBoardsMap,
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
