<template>
  <div class="kf-journal-events__wrap">
    <div class="kf-journal-filters-bar">
      <div class="kf-journal-bar-title">
        {{ `${$t('journalConfig.filters')}: ` }}
      </div>
      <FrameFilters
        ref="frameFilter"
        @apply-filters="onFiltersApply"
      ></FrameFilters>
    </div>
    <div class="kf-journal-frame__wrap">
      <KfTradingDataTable
        v-model:selected-key="currentFramesKey"
        :selectable="true"
        :data-source="frameDataListResolved"
        :columns="frameColumns"
        key-field="genTime"
        :resizable="false"
        @click-cell="handleOpenFrameDetail"
      >
        <template
          #default="{
            item,
            column,
          }: {
            item: KungfuApi.FrameResolved,
            column: KfTradingDataTableHeaderConfig,
          }"
        >
          <template v-if="column.dataIndex === 'stringMsgType'">
            <a-tag :color="item.msgTypeResolved.color">
              {{ item.stringMsgType }}
            </a-tag>
          </template>
          <template v-else>
            <span>
              {{ item[column.dataIndex as keyof KungfuApi.FrameResolved] }}
            </span>
          </template>
        </template>
      </KfTradingDataTable>
    </div>
    <a-spin
      class="kf-journal-spin"
      :spinning="loadingJournal"
      :tip="$t('journalConfig.loading_journal')"
    />
    <a-drawer
      v-model:visible="visible"
      title="Event Frame"
      placement="right"
      :force-render="true"
    >
      <template v-if="currentRowDataResolved">
        <a-list size="normal" bordered :data-source="currentRowDataResolved">
          <template #renderItem="{ item }">
            <a-list-item>
              <span class="frame-detail-datalist-key">{{ item.key }}</span>
              <span class="frame-detail-datalist-value">
                {{ item.value }}
              </span>
            </a-list-item>
          </template>
        </a-list>

        <a-card title="Frame Data" style="margin: 35px 0">
          <a-tree
            :show-line="true"
            :show-icon="true"
            :tree-data="framesMap[currentFramesKey].dataResolved"
            :selectable="false"
            default-expand-all
          ></a-tree>
        </a-card>
      </template>
      <template v-else>
        <a-empty :image="simpleImage"></a-empty>
      </template>
    </a-drawer>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, watch, toRaw, shallowRef } from 'vue';
import { Empty } from 'ant-design-vue';
import { assemble } from '@kungfu-trader/kungfu-js-api/kungfu';
import { getFrameColumns } from '../config';
import { dealFrame } from '../utils';
import { createFiltersEnumMap, FiltersEnum } from '../utils/filterUtils';
import KfTradingDataTable from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfTradingDataTable.vue';
import FrameFilters from './FrameFilters.vue';

const props = withDefaults(
  defineProps<{
    currentSessionId: number;
    sessionLocationMap: Record<number, KungfuApi.KfLocation>;
  }>(),
  {
    sessionLocationMap: () => ({}),
  },
);

const frameColumns = getFrameColumns();
const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;
const loadingJournal = ref(false);

const currentFramesKey = ref('');
const frameFilter = ref();

const frameDataList = shallowRef<KungfuApi.FrameResolved[]>([]);
const frameFiltersReg = ref(createFiltersEnumMap(/.*/));
const frameDataListResolved = computed(() => {
  return frameDataList.value.filter((item) => {
    return Object.keys(frameFiltersReg.value).every((filterKey) => {
      if (frameFiltersReg.value[filterKey]) {
        const curReg = frameFiltersReg.value[filterKey as FiltersEnum];
        switch (filterKey) {
          case FiltersEnum.DEST:
            return curReg.test(item.dest + '');
          case FiltersEnum.SOURCE:
            return curReg.test(item.source + '');
          case FiltersEnum.MSG_TYPE:
            return curReg.test(item.msgType + '');
        }

        return true;
      } else {
        return true;
      }
    });
  });
});

const framesMap = shallowRef<Record<string, KungfuApi.FrameResolved>>({});

const visible = ref(false);
const currentRowDataResolved = computed(() => {
  const currentRowData = framesMap.value[currentFramesKey.value];
  if (currentRowData) {
    return Object.keys(currentRowData)
      .map((item) => {
        if (item.indexOf('Resolved') !== -1 || item === 'data') return null;

        const key = currentRowData[`${item}Resolved`]
          ? `${item}Resolved`
          : item;

        const value =
          item === 'msgType'
            ? currentRowData.msgType
            : `${currentRowData[key]}`;

        return {
          key: item as unknown as keyof KungfuApi.FrameResolved,
          value,
        };
      })
      .filter((item) => !!item);
  }

  return null;
});

watch(
  () => props.currentSessionId,
  (newId, oldId) => {
    if (newId !== -1 && newId !== oldId) {
      loadFrameData(newId);
    }
  },
);

let timer = 0;
let journalReader: KungfuApi.AssembleReader | null = null;

const loadFrameData = (sessionId: number, checking = false) => {
  console.log('sessionId', sessionId);

  if (checking && timer) {
    clearTimeout(timer);
  } else {
    loadingJournal.value = true;
    journalReader = assemble.get_reader(sessionId);
    framesMap.value = {};
  }

  let count = 0;
  const curFramesMap = {};

  return new Promise<KungfuApi.FrameResolved[]>((resolve, _) => {
    const runner = () => {
      setTimeout(() => {
        if (!journalReader) return;
        const isUnfinish = journalReader.next();

        if (isUnfinish) {
          const frame = journalReader.currentFrame();
          ++count;

          const curFrameData: KungfuApi.Frame = {
            dataLength: frame.dataLength(),
            genTime: frame.genTime(),
            triggerTime: frame.triggerTime(),
            msgType: frame.msgType(),
            stringMsgType: frame.stringMsgType(),
            source: frame.source(),
            dest: frame.dest(),
            data: frame.data(),
          };

          if (!(`${curFrameData.genTime}` in curFramesMap)) {
            const curFrameDataResolved = dealFrame(
              curFrameData,
              props.sessionLocationMap,
            );

            curFramesMap[`${curFrameData.genTime}`] = curFrameDataResolved;

            frameFilter.value?.addOption(FiltersEnum.DEST, {
              label: curFrameDataResolved.destResolved,
              value: curFrameDataResolved.dest + '',
            });
            frameFilter.value?.addOption(FiltersEnum.SOURCE, {
              label: curFrameDataResolved.sourceResolved,
              value: curFrameDataResolved.source + '',
            });
            frameFilter.value?.addOption(FiltersEnum.MSG_TYPE, {
              label: curFrameDataResolved.stringMsgType,
              value: curFrameDataResolved.msgType + '',
            });
          }

          runner();
        } else {
          Object.keys(curFramesMap).forEach((item) => {
            if (!(item in framesMap.value)) {
              framesMap.value[item] = curFramesMap[item];
            }
          });

          resolve(Object.values(toRaw(curFramesMap)));
        }
      });
    };

    runner();
  }).then((res) => {
    console.log(res);
    console.log(count);

    if (checking) {
      frameDataList.value.push(...res);
    } else {
      frameDataList.value = res;
      currentFramesKey.value = frameDataList.value[0]?.genTime + '';

      loadingJournal.value = false;
    }

    timer = window.setTimeout(() => {
      loadFrameData(sessionId, true);
    }, 500);
  });
};

const handleOpenFrameDetail = () => {
  visible.value = true;
};

const onFiltersApply = (filtersFormState: Record<FiltersEnum, string[]>) => {
  Object.keys(filtersFormState).forEach((key) => {
    frameFiltersReg.value[key] = new RegExp(
      `^${
        filtersFormState[key].reduce<string>(
          (str: string, cur: string) => str + `(${cur})?`,
          '',
        ) || '.*'
      }$`,
    );
  });
};

defineExpose({
  frameDataList,
});
</script>

<style lang="less">
.kf-journal-events__wrap {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;

  .kf-journal-spin {
    .ant-spin-text {
      margin-left: 8px;
    }
  }

  .kf-journal-filters-bar {
    flex: 0 40px;
    height: 40px;
    background-color: #1d1d1d;
    padding: 5px 20px;
    margin-bottom: 2px;
    display: flex;
    align-items: center;
    justify-content: flex-start;

    .kf-journal-bar-title {
      font-size: 14px;
      margin-right: 16px;
    }
  }

  .kf-journal-frame__wrap {
    flex: auto;
  }
}

.ant-drawer-body {
  margin-bottom: 8px;

  .ant-tree-switcher-noop {
    display: none;
  }
}
</style>
