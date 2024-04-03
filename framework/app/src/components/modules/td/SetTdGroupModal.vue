<script lang="ts" setup>
import { setTdGroup } from '@kungfu-trader/kungfu-js-api/actions';
import { getIdByKfLocation } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import {
  getInstrumentTypeColor,
  isInTdGroup,
  useModalVisible,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { useCurrentGlobalKfLocation } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';
import { AntTreeNodeDropEvent, DataNode } from 'ant-design-vue/lib/tree';
import { computed, ComputedRef, toRaw, toRefs } from 'vue';
import { useGlobalStore } from '@kungfu-trader/kungfu-app/src/renderer/pages/index/store/global';
import {
  useAllKfConfigData,
  useExtConfigsRelated,
  useTdGroups,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';

const props = withDefaults(
  defineProps<{
    visible: boolean;
  }>(),
  {
    visible: false,
  },
);

defineEmits<{
  (e: 'update:visible', visible: boolean): void;
  (e: 'close'): void;
}>();

const { currentGlobalKfLocation, setCurrentGlobalKfLocation } =
  useCurrentGlobalKfLocation(window.watcher);

const { modalVisible, closeModal } = useModalVisible(props.visible);
const { tdExtTypeMap } = useExtConfigsRelated();
const tdGroup = useTdGroups();
const { td } = toRefs(useAllKfConfigData());

const tdTreeData: ComputedRef<DataNode[]> = computed(() => {
  const tdGroupResolved: Record<string, DataNode> = {};
  const tdResolved: DataNode[] = [];
  const markedNameToTdGroup: Record<string, KungfuApi.KfExtraLocation> = {};
  [...tdGroup.value, ...td.value].forEach((item) => {
    if ('children' in item) {
      markedNameToTdGroup[item.name] = { ...item };
      tdGroupResolved[item.name] = {
        ...transformKfConfigToDataNode(item),
        children: [],
      };
      return;
    }

    const accountId = `${item.group}_${item.name}`;
    const targetGroupNames = Object.keys(markedNameToTdGroup).filter((name) => {
      const tdGroup = markedNameToTdGroup[name];
      return (tdGroup.children || []).includes(accountId);
    });
    if (targetGroupNames.length) {
      const targetGroupName = targetGroupNames[0];
      tdGroupResolved[targetGroupName].children?.push(
        transformKfConfigToDataNode(item),
      );
      return;
    }

    tdResolved.push(transformKfConfigToDataNode(item));
  });
  return [...Object.values(tdGroupResolved), ...tdResolved];
});

function transformKfConfigToDataNode(
  target: KungfuApi.KfConfig | KungfuApi.KfExtraLocation,
): DataNode {
  return {
    name: target.name,
    title: target.name,
    category: target.category,
    group: target.group,
    mode: target.mode,
    key: `${target.category}_${target.group}_${target.name}`,
  };
}

function isGroup(node: DataNode): KungfuApi.KfExtraLocation | null {
  if ('children' in node) {
    const name = node.name;
    const targetGroups = tdGroup.value.filter((item) => {
      return item.name === name;
    });
    if (targetGroups.length) {
      return targetGroups[0];
    } else {
      return null;
    }
  } else {
    return null;
  }
}

function setGlobalLocation() {
  if (
    currentGlobalKfLocation.value?.category === 'tdGroup' &&
    tdTreeData.value.length > 0
  ) {
    for (const item of tdTreeData.value) {
      if (item.name === currentGlobalKfLocation.value.name) {
        setCurrentGlobalKfLocation(
          item as unknown as KungfuApi.KfExtraLocation,
        );
        break;
      }
    }
  }
}

function setTdGroupAndGlobalLocation() {
  setTdGroup(toRaw(tdGroup.value)).then(() => {
    useGlobalStore().setTdGroups();
    setGlobalLocation();
  });
}

function handleDrop(info: AntTreeNodeDropEvent) {
  const { dragNode, node, dropPosition, dropToGap } = info;

  if (isGroup(dragNode)) {
    return;
  }

  const targetAccountId = getIdByKfLocation(
    dragNode as unknown as KungfuApi.KfExtraLocation,
  );
  const oldGroup = isInTdGroup(tdGroup.value, targetAccountId);

  if (oldGroup) {
    const oldTargetIndex = oldGroup.children.indexOf(targetAccountId);
    oldGroup.children.splice(oldTargetIndex, 1); //remove from old
  }

  const group = isGroup(node);
  if (group) {
    if (dropPosition === -1) {
      setTdGroupAndGlobalLocation();
      return;
    }
    const groupIndex = tdGroup.value.findIndex(
      (group) => node.name === group.name,
    );
    //from group to bottom
    if (!dropToGap || dropPosition !== groupIndex + 1) {
      group.children.push(targetAccountId);
    }
  }

  const destAccountId = getIdByKfLocation(
    node as unknown as KungfuApi.KfExtraLocation,
  );
  const newGroup = isInTdGroup(tdGroup.value, destAccountId);
  if (newGroup) {
    newGroup.children.push(targetAccountId);
  }

  setTdGroupAndGlobalLocation();
}

getInstrumentTypeColor;
</script>
<template>
  <a-modal
    :width="420"
    class="set-td-group-modal"
    v-model:visible="modalVisible"
    :title="$t('tdConfig.set_td_group')"
    :destroyOnClose="true"
    :footer="null"
    @cancel="closeModal"
  >
    <a-tree
      v-if="tdTreeData.length"
      draggable
      :tree-data="tdTreeData"
      :blockNode="true"
      :defaultExpandAll="true"
      @drop="handleDrop"
    >
      <template #title="{ dataRef }">
        <div v-if="dataRef.category === 'td'" class="set-td-name__warp">
          <a-tag
            class="kf-td-tree-tag"
            size="small"
            :color="getInstrumentTypeColor(tdExtTypeMap[dataRef.group])"
          >
            {{ dataRef.group }}
          </a-tag>
          {{ dataRef.name }}
        </div>
        <div
          v-else-if="dataRef.category === 'tdGroup'"
          class="set-td-name__warp"
        >
          <a-tag size="small" color="#FAAD14">
            {{ $t('tdConfig.account_group') }}
          </a-tag>
          {{ dataRef.name }}
        </div>
      </template>
    </a-tree>
  </a-modal>
</template>
<style lang="less">
.set-td-group-modal {
  .kf-td-tree-tag {
    display: inline-block;
  }
  .set-td-name__warp {
    word-break: break-all;
  }
}
</style>
