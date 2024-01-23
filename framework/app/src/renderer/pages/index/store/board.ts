import { StoreDefinition, defineStore, _UnwrapAll } from 'pinia';
import {
  KfLayoutDirection,
  KfLayoutTargetDirectionClassName,
} from '@kungfu-trader/kungfu-app/src/typings/enums';
import {
  onDeactivated,
  onActivated,
  onUnmounted,
  ref,
  toRaw,
  getCurrentInstance,
  Ref,
} from 'vue';
import { Subscription } from 'rxjs';
import { messagePrompt } from '../../../assets/methods/uiUtils';

// 对应store中的state
interface StateTree {
  boardsMap: Ref<KfLayout.BoardsMap>;
  dragedContentData: Ref<KfLayout.ContentData | null>;
  isBoardDragging: Ref<boolean>;
}
// 对应store中的action
interface ActionsTree {
  markIsBoardDragging: (status: boolean) => void;
  initBoardsMap: (boardMap: KfLayout.BoardsMap) => void;
  setBoardsMapAttrById: (
    id: number,
    attrKey: keyof KfLayout.BoardInfo,
    value: KfLayout.BoardInfo[keyof KfLayout.BoardInfo],
  ) => void;
  addBoardFromEmpty: (targetContentId: string) => Promise<void>;
  addBoardByContentId: (
    targetBoardId: number,
    targetContentId: string,
  ) => Promise<void>;
  removeBoardByContentId: (
    targetBoardId: number,
    targetContentId: string,
  ) => void;
  setDragedContentData: (
    boardId: KfLayout.BoardId,
    contentId: KfLayout.ContentId,
  ) => void;
  afterDragMoveBoard: (
    dragedContentData: KfLayout.ContentData | null,
    destBoardId: KfLayout.BoardId,
    directionClassName: KfLayoutTargetDirectionClassName,
  ) => void;
  saveBoardsMap: () => Promise<void>;
}

type combineType = StateTree & ActionsTree;
declare global {
  interface Window {
    allBoardsStore: Record<
      string,
      StoreDefinition<
        `${string}_boardsStore`,
        _UnwrapAll<Pick<combineType, keyof StateTree>>,
        Pick<combineType, never>, // never在有computed的时候用，对应store中getter
        Pick<combineType, keyof ActionsTree>
      >
    >;
  }
}

export const useBoards = () => {
  const app = getCurrentInstance();
  const { success } = messagePrompt();

  const createBoardsStore = (
    boardsStoreId: string,
    initBoardMap: KfLayout.BoardsMap,
    defaultBoardsMap: KfLayout.BoardsMap,
  ) => {
    const useBoardsStore = defineStore(`${boardsStoreId}_boardsStore`, () => {
      const boardsMap = ref<KfLayout.BoardsMap>(initBoardMap);
      const dragedContentData = ref<KfLayout.ContentData | null>(null);
      const isBoardDragging = ref<boolean>(false);

      const localBoardsMapKey = `${boardsStoreId}_boardsMap`;

      let subscription: Subscription | undefined;
      onActivated(() => {
        subscription = app?.proxy?.$globalBus.subscribe(
          (data: KfEvent.KfBusEvent) => {
            if (data.tag === 'main') {
              if (data.name === 'reset-main-dashboard') {
                initBoardsMap(defaultBoardsMap);
                success();
              }

              if (data.name == 'record-before-quit') {
                Object.values(window.allBoardsStore).forEach((store) => {
                  store().saveBoardsMap();
                });
              }
            }
          },
        );
      });

      onDeactivated(() => {
        subscription && subscription.unsubscribe();
      });

      onUnmounted(() => {
        subscription && subscription.unsubscribe();
        saveBoardsMap();
      });

      function markIsBoardDragging(status: boolean) {
        isBoardDragging.value = status;
      }

      function initBoardsMap(boardMap: KfLayout.BoardsMap) {
        boardsMap.value = JSON.parse(JSON.stringify(boardMap));
      }

      function setBoardsMapAttrById(
        id: number,
        attrKey: keyof KfLayout.BoardInfo,
        value: KfLayout.BoardInfo[keyof KfLayout.BoardInfo],
      ) {
        (<typeof value>boardsMap.value[id][attrKey]) = value;
      }

      function addBoardFromEmpty(targetContentId: string) {
        const newBoardInfo: KfLayout.BoardInfo = {
          paId: 0,
          direction: KfLayoutDirection.v,
          contents: [targetContentId],
          current: targetContentId,
          width: '100%',
          height: '100%',
        };
        boardsMap.value[1] = newBoardInfo;
        boardsMap.value[0].children = [1];
        return Promise.resolve();
      }

      function addBoardByContentId(
        targetBoardId: number,
        targetContentId: string,
      ): Promise<void> {
        const targetBoard: KfLayout.BoardInfo = boardsMap.value[targetBoardId];
        const contents = targetBoard?.contents;
        const targetIndex = contents?.indexOf(targetContentId);

        if (contents === undefined) {
          return Promise.reject();
        } else if (targetIndex === undefined) {
          return Promise.reject();
        } else if (targetIndex !== -1) {
          return Promise.reject();
        }

        contents.push(targetContentId);
        targetBoard.current = targetContentId;

        return Promise.resolve();
      }

      function removeBoardByContentId(
        targetBoardId: number,
        targetContentId: string,
      ) {
        const targetBoard: KfLayout.BoardInfo = boardsMap.value[targetBoardId];
        const contents = targetBoard?.contents;
        const targetIndex = contents?.indexOf(targetContentId);

        if (targetIndex === undefined) return;
        if (targetIndex === -1) return;

        const removedItem: KfLayout.ContentId =
          (contents?.splice(targetIndex, 1) || [])[0] || '';

        if (removedItem === targetBoard.current && contents?.length) {
          targetBoard.current = (targetBoard.contents || [])[0];
        }

        if (!contents?.length && targetBoard.paId != -1) {
          removeBoardByBoardId_(targetBoardId);
        }
      }

      function removeBoardByBoardId_(targetBoardId: number) {
        const targetBoard = boardsMap.value[targetBoardId];
        if (targetBoard && targetBoard.paId !== -1) {
          const paId = targetBoard.paId;
          const paBoard = boardsMap.value[paId];
          const children = paBoard?.children;
          const childIndex = paBoard.children?.indexOf(targetBoardId);

          if (childIndex === undefined) return;
          if (childIndex === -1) return;

          children?.splice(childIndex, 1);

          if (!children?.length) {
            removeBoardByBoardId_(paId);
          } else {
            children.forEach((childId: KfLayout.BoardId) => {
              boardsMap.value[childId].width = 0;
              boardsMap.value[childId].height = 0;
            });
          }

          delete boardsMap.value[targetBoardId];
        }
        return;
      }

      function setDragedContentData(
        boardId: KfLayout.BoardId,
        contentId: KfLayout.ContentId,
      ) {
        if (boardId === -1 && !contentId) {
          dragedContentData.value = null;
        } else {
          dragedContentData.value = {
            contentId,
            boardId,
          };
        }
      }

      function afterDragMoveBoard(
        dragedContentData: KfLayout.ContentData | null,
        destBoardId: KfLayout.BoardId,
        directionClassName: KfLayoutTargetDirectionClassName,
      ) {
        const { boardId, contentId } = dragedContentData || {};
        const destBoard = boardsMap.value[destBoardId];

        if (!contentId || boardId === undefined) return;

        //to self
        if (
          boardId === destBoardId &&
          destBoard.contents &&
          destBoard.contents.length === 1
        ) {
          return;
        }

        removeBoardByContentId(boardId, contentId);

        if (directionClassName === KfLayoutTargetDirectionClassName.center) {
          if (destBoard.contents) {
            if (!destBoard.contents.includes(contentId)) {
              destBoard.contents.push(contentId);
            }
            destBoard.current = contentId;
          }
        } else if (
          directionClassName != KfLayoutTargetDirectionClassName.unset
        ) {
          dragMakeNewBoard_(contentId, destBoardId, directionClassName);
        }
      }

      function dragMakeNewBoard_(
        contentId: KfLayout.ContentId,
        destBoardId: number,
        directionClassName: KfLayoutTargetDirectionClassName,
      ) {
        const destBoard = boardsMap.value[destBoardId];
        const destPaId: number = destBoard.paId;
        const destDirection: KfLayoutDirection = destBoard.direction;
        const newBoardId: KfLayout.BoardId = buildNewBoardId_();

        const newBoardDirection: KfLayoutDirection =
          directionClassName === KfLayoutTargetDirectionClassName.top ||
          directionClassName === KfLayoutTargetDirectionClassName.bottom
            ? KfLayoutDirection.h
            : directionClassName === KfLayoutTargetDirectionClassName.left ||
              directionClassName === KfLayoutTargetDirectionClassName.right
            ? KfLayoutDirection.v
            : KfLayoutDirection.unset;
        const newBoardInfo: KfLayout.BoardInfo = {
          paId: destPaId,
          direction: newBoardDirection,
          contents: [contentId],
          current: contentId,
        };

        if (destDirection === newBoardDirection) {
          const siblings = boardsMap.value[destPaId].children;
          const destIndex = siblings?.indexOf(destBoardId);
          if (destIndex === -1 || destIndex === undefined) {
            throw new Error("Insert dest board is not in pa board's childen");
          }

          if (
            directionClassName === KfLayoutTargetDirectionClassName.top ||
            directionClassName === KfLayoutTargetDirectionClassName.left
          ) {
            siblings?.splice(destIndex, 0, newBoardId);
          } else {
            siblings?.splice(destIndex + 1, 0, newBoardId);
          }
        } else {
          newBoardInfo.paId = destBoardId;
          const destBoardCopy: KfLayout.BoardInfo = {
            ...toRaw(destBoard),
            direction: newBoardDirection,
            paId: destBoardId,
            width: undefined,
            height: undefined,
          };

          const newDestBoardId = newBoardId + 1;
          if (
            directionClassName === KfLayoutTargetDirectionClassName.top ||
            directionClassName === KfLayoutTargetDirectionClassName.left
          ) {
            destBoard.children = [newBoardId, newDestBoardId];
          } else {
            destBoard.children = [newDestBoardId, newBoardId];
          }
          delete destBoard.contents;
          delete destBoard.current;

          boardsMap.value[newDestBoardId] = destBoardCopy;
        }

        destBoard.width && delete destBoard.width;
        destBoard.height && delete destBoard.height;

        boardsMap.value[newBoardId] = newBoardInfo;
      }

      function buildNewBoardId_(): KfLayout.BoardId {
        const boardIds = Object.keys(boardsMap.value)
          .map((key: string) => +key)
          .sort((key1: number, key2: number) => key2 - key1);
        return boardIds[0] + 1;
      }

      function saveBoardsMap(): Promise<void> {
        localStorage.setItem(
          localBoardsMapKey,
          JSON.stringify(boardsMap.value || '{}'),
        );
        return Promise.resolve();
      }

      return {
        boardsMap,
        dragedContentData,
        isBoardDragging,

        markIsBoardDragging,
        initBoardsMap,
        setBoardsMapAttrById,
        addBoardFromEmpty,
        addBoardByContentId,
        removeBoardByContentId,
        setDragedContentData,
        afterDragMoveBoard,
        saveBoardsMap,
      } as combineType;
    });

    window.allBoardsStore[boardsStoreId] = useBoardsStore;

    return useBoardsStore;
  };

  const getBoardsStoreById = (boardsStoreId: string) => {
    return window.allBoardsStore[boardsStoreId];
  };

  const getLocalBoardsMap = (
    boardsStoreId: string,
  ): KfLayout.BoardsMap | null => {
    const data = localStorage.getItem(`${boardsStoreId}_boardsMap`);
    if (!data) {
      return null;
    }

    const storedBoardsMap = JSON.parse(data) as KfLayout.BoardsMap;
    if (!Object.keys(storedBoardsMap).length) {
      return null;
    }

    return storedBoardsMap;
  };

  return {
    getBoardsStoreById,
    createBoardsStore,
    getLocalBoardsMap,
  };
};
