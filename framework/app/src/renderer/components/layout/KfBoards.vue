<template>
  <KfRowColIter
    :board-id="0"
    :closable="closable"
    :init-boards-map="curBoardsMap"
    :current-boards-store-id="boardsId"
    :default-boards-map="curDefaultBoardsMap"
  ></KfRowColIter>
</template>

<script lang="ts" setup>
import { getCurrentInstance, onActivated, onDeactivated, Component } from 'vue';

import KfRowColIter from '@kungfu-trader/kungfu-app/src/renderer/components/layout/KfRowColIter.vue';
import { registerComponents } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { useBoards } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/board';

const props = defineProps<{
  boardsId: string;
  closable?: boolean;
  boardsMapBuilder: () => KfLayout.BoardsMap;
  componentsMap?: { [componentsName: string]: Component };
}>();

const app = getCurrentInstance();
const { getLocalBoardsMap, createBoardsStore } = useBoards();
const curDefaultBoardsMap = props.boardsMapBuilder();
const curBoardsMap: KfLayout.BoardsMap =
  getLocalBoardsMap(props.boardsId) || curDefaultBoardsMap;

createBoardsStore(props.boardsId, curBoardsMap, curDefaultBoardsMap);

if (app && props.componentsMap) {
  registerComponents(app.appContext.app, props.componentsMap);
}

onActivated(() => {
  const subscription = app?.proxy?.$globalBus.subscribe(
    (data: KfEvent.KfBusEvent) => {
      if (data.name == 'record-before-quit') {
        window.watcher && window.watcher.quit();
      }
    },
  );

  onDeactivated(() => {
    subscription?.unsubscribe();
  });
});
</script>

<style lang="less"></style>
