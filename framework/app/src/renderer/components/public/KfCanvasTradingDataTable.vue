<template>
  <div
    ref="listTableRef"
    style="width: 100%; height: 100%; margin-top: -1px"
  ></div>
  <a-empty
    v-if="showEmpty"
    ref="emptyRef"
    :image="simpleImage"
    :description="t('empty_text')"
  ></a-empty>
</template>

<script setup lang="ts">
import {
  onMounted,
  ref,
  watch,
  getCurrentInstance,
  computed,
  nextTick,
} from 'vue';
import { Empty } from 'ant-design-vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

import {
  VTable,
  ICustomActionOption,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable';

const { t } = VueI18n.global;

const app = getCurrentInstance();
const simpleImage = Empty.PRESENTED_IMAGE_SIMPLE;
const showEmpty = ref<boolean>(false);
let widthMode: 'adaptive' | 'autoWidth' | 'standard' = 'standard';
let columnResizeMode: 'all' | 'body' | 'header' | 'none' = 'none';
let font = '';
const ColumnCustomMap = ref<
  Record<string, { customLayout: VTable.TYPES.ICustomLayoutFuc }>
>({});

type tableDataItem =
  | KungfuApi.TradingDataItem
  | KungfuApi.Frame
  | KungfuApi.Session;

const props = withDefaults(
  defineProps<{
    columns: VTable.ColumnsDefine;
    dataSource?: tableDataItem[];
    hasData?: boolean;
    customLayout?: Record<string, ICustomActionOption[]>;
    widthMode?: 'adaptive' | 'autoWidth' | 'standard';
    columnResizeMode?: 'all' | 'body' | 'header' | 'none';
    optionItems?: VTable.ListTableConstructorOptions;
    event?: Partial<VTable.TYPES.TableEventHandlersEventArgumentMap>;
    ScrollableContainerWidth?: number;
  }>(),
  {
    columns: () => [],
    optionItems: () => ({}),
    dataSource: () => [],
    event: () => ({}),
  },
);

defineEmits<{
  (
    e: 'clickCell',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['click_cell'],
  ): void;
  (
    e: 'dblclickCell',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['dblclick_cell'],
  ): void;
  (
    e: 'rightClickRow',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['contextmenu_cell'],
  ): void;
  (
    e: 'mouseenterTable',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['mouseenter_table'],
  ): void;
  (
    e: 'mouseleaveTable',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['mouseleave_table'],
  ): void;
  (
    e: 'mouseenterCell',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['mouseenter_cell'],
  ): void;
  (
    e: 'mouseleaveCell',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['mouseleave_cell'],
  ): void;
  (
    e: 'mousemoveCell',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['mousemove_cell'],
  ): void;
  (
    e: 'mousedownCell',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['mousedown_cell'],
  ): void;
  (
    e: 'mouseupCell',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['mouseup_cell'],
  ): void;
  (
    e: 'keydown',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['keydown'],
  ): void;
  (
    e: 'scroll',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['scroll'],
  ): void;
  (
    e: 'checkboxStateChange',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['checkbox_state_change'],
  ): void;
  (
    e: 'resizeColumn',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['resize_column'],
  ): void;
  (
    e: 'resizeColumnEnd',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['resize_column_end'],
  ): void;
  (
    e: 'changeHeaderPosition',
    data: VTable.TYPES.TableEventHandlersEventArgumentMap['change_header_position'],
  ): void;
}>();

const defaultTheme: VTable.TYPES.ITableThemeDefine = {
  columnResize: {
    lineColor: 'transparent',
    bgColor: 'transparent',
    lineWidth: 0,
    labelColor: 'transparent',
    labelFontSize: 0,
    labelFontFamily: 'Monospace, sans-serif',
    labelBackgroundFill: 'transparent',
  },
  underlayBackgroundColor: 'transparent',
  bodyStyle: {
    bgColor: 'transparent',
    autoWrapText: true,
    textBaseline: 'alphabetic',
  },
  headerStyle: {
    bgColor: '#1d1d1d',
    borderLineDash: [1, 1],
    borderLineWidth: 1,
    borderColor: '#141414',
    color: '#ffffffd9',
    // lineHeight: 35,
    hover: {
      cellBgColor: '#333',
      inlineRowBgColor: '#333',
    },
    // cursor: 'pointer',
    textBaseline: 'middle',
  },
  defaultStyle: {
    borderLineWidth: 0,
    bgColor: 'transparent',
    color: '#ffffffd9',
    fontSize: 12,
    autoWrapText: true,
    hover: {
      cellBgColor: '#333',
      inlineRowBgColor: '#333',
    },
    fontWeight: 100,
    fontFamily: 'Monospace, sans-serif',
  },
  tooltipStyle: {
    bgColor: '#333',
    color: '#ffffffd9',
    fontSize: 12,
    fontFamily: 'Monospace, sans-serif',
    padding: [4, 4, 4, 4],
  },
  scrollStyle: {
    scrollSliderColor: '#555',
    visible: 'focus',
  },
  checkboxStyle: {
    size: 12,
    spaceBetweenTextAndIcon: 4,
    defaultFill: 'transparent',
    defaultStroke: '#444',
    disableFill: '#444',
    checkedFill: '#FAAD14',
    checkedStroke: '#FAAD14',
    disableCheckedFill: '#FAAD14',
    disableCheckedStroke: '#FAAD14',
  },
  selectionStyle: {
    cellBgColor: 'rgba(128, 128, 128, 0.3)',
    cellBorderColor: '#444',
    cellBorderLineWidth: 2,
  },
  dragHeaderSplitLine: {
    lineColor: '#FAAD14',
    lineWidth: 1,
    // shadowBlockColor: 'rgba(128, 128, 128, 0.3)',
  },
};

const defaultOptionItems = ref<VTable.ListTableConstructorOptions>({
  theme: defaultTheme,
  hover: {
    highlightMode: 'row',
  },
  select: {
    disableSelect: true,
  },
  maintainedDataCount: 100,
  defaultRowHeight: 30,
  columnResizeMode,
  widthMode,
  limitMaxAutoWidth: 300,
  //  autoFillHeight:true,
  //  frozenColCount: 1,
  //  rightFrozenColCount: 1,
  tooltip: {
    isShowOverflowTextTooltip: true,
  },
});
const listTableRef = ref();
const emptyRef = ref();
const option = computed<VTable.ListTableConstructorOptions>(() => {
  return {
    columns: props.columns,
    ...defaultOptionItems.value,
    ...props.optionItems,
  } as VTable.ListTableConstructorOptions;
});
let listTable: VTable.ListTable | null = null;

const containerWidth = ref<number>(10);

const initCustomLayoutOptions = () => {
  if (!props.customLayout) return;
  const customLayoutOption = props.customLayout;
  Object.keys(customLayoutOption).forEach((key) => {
    if (!customLayoutOption[key]) return;
    ColumnCustomMap.value[key] = {
      customLayout: (args: VTable.TYPES.CustomRenderFunctionArg) => {
        const { table, row, col, rect } = args;
        const { height, width } = rect || table.getCellRect(col, row);
        const record = table.getRecordByCell(col, row);

        const container = new VTable.CustomLayout.Group({
          height,
          width,
          display: 'flex',
          flexDirection: 'row',
          alignItems: 'center',
          flexWrap: 'nowrap',
          alignContent: 'center',
        });
        let obj;
        for (obj of customLayoutOption[key]) {
          const { type, dealValue, ...rest } = obj;
          if (type === 'text') {
            try {
              const text =
                dealValue && typeof dealValue === 'function'
                  ? dealValue(record)
                  : dealValue;
              if (text) {
                const customLayout = new VTable.CustomLayout.Text({
                  ...rest,
                  text,
                });
                container.add(customLayout);
              }
            } catch (error) {
              console.log(error);
            }
          } else if (type === 'image') {
            try {
              const image =
                dealValue && typeof dealValue === 'function'
                  ? dealValue(record)
                  : dealValue;
              if (image) {
                const customLayout = new VTable.CustomLayout.Image({
                  ...rest,
                  image,
                });
                container.add(customLayout);
              }
            } catch (error) {
              console.log(error);
            }
          }
        }

        return {
          rootContainer: container,
          renderDefault: false,
        };
      },
    };
  });
  option.value.columns?.forEach((column) => {
    if (
      column.field &&
      typeof column.field === 'string' &&
      ColumnCustomMap.value[column.field]
    ) {
      column.customLayout = ColumnCustomMap.value[column.field].customLayout;
    }
  });
};

const isShowEmpty = () => {
  if (listTable) {
    if (!props.hasData) {
      nextTick(() => {
        listTableRef.value.style.height = `35px`;
        if (
          defaultTheme.scrollStyle &&
          defaultTheme.scrollStyle.visible !== 'none'
        ) {
          defaultTheme.scrollStyle.visible = 'none';
          listTable?.updateTheme(defaultTheme);
        }
        listTable?.setRecords([]);
        showEmpty.value = true;
      });
    } else {
      listTableRef.value.style.height = `100%`;
      if (
        defaultTheme.scrollStyle &&
        defaultTheme.scrollStyle.visible !== 'focus'
      ) {
        defaultTheme.scrollStyle.visible = 'focus';
        listTable.updateTheme(defaultTheme);
      }
      showEmpty.value = false;
    }
  }
};

onMounted(() => {
  font = document.body.style.fontFamily;
  if (font) {
    if (defaultTheme.defaultStyle) {
      defaultTheme.defaultStyle.fontFamily = font;
    }
    if (defaultTheme.columnResize) {
      defaultTheme.columnResize.labelFontFamily = font;
    }
    if (defaultTheme.tooltipStyle) {
      defaultTheme.tooltipStyle.fontFamily = font;
    }
  }
  initCustomLayoutOptions();
  widthMode = props.widthMode || 'standard';
  columnResizeMode = props.columnResizeMode || 'none';
  defaultOptionItems.value.widthMode = widthMode;
  defaultOptionItems.value.columnResizeMode = columnResizeMode;
  if (listTableRef.value) {
    listTable = new VTable.ListTable(
      listTableRef.value,
      option.value as VTable.ListTableConstructorOptions,
    );
    isShowEmpty();
  }

  const rowList = listTable?.getAllColumnHeaderCells();
  if (rowList && rowList[0]) {
    containerWidth.value = rowList[0].reduce((pre, cur) => {
      return pre + Number(cur?.cellRange?.width);
    }, 0);
  }
  registerEvent();
  if (listTableRef.value?.parentNode) {
    new ResizeObserver((entries) => {
      if (!listTable) return;
      const { width } = entries[0].contentRect;
      const defaultWidth =
        props.ScrollableContainerWidth || containerWidth.value;
      if (!defaultWidth) return;
      if (width < defaultWidth && listTable.widthMode === 'adaptive') {
        listTable.widthMode = widthMode;
        listTable.renderWithRecreateCells();
      } else if (width >= defaultWidth && listTable.widthMode !== 'adaptive') {
        listTable.widthMode = 'adaptive';
        listTable.renderWithRecreateCells();
      }
    }).observe(listTableRef.value?.parentNode as HTMLElement);
  }
});
const getListTable = () => {
  return listTable;
};

const setRecords = (records: tableDataItem[]) => {
  nextTick(() => {
    if (listTable) {
      listTable?.setRecords(records);
    }
  });
};

defineExpose({
  setRecords,
  getListTable,
  initCustomLayoutOptions,
});

watch(
  () => props.hasData,
  () => {
    isShowEmpty();
  },
  { immediate: true },
);

watch(
  () => props.customLayout,
  (customLayout) => {
    if (customLayout && listTable) {
      initCustomLayoutOptions();
      listTable.updateOption(option.value);
    }
  },
);

watch(
  () => props.columns,
  () => {
    if (listTable) {
      initCustomLayoutOptions();
      listTable.updateOption(option.value);
    }
  },
);

const registerEvent = () => {
  if (!listTable) return;

  const eventMap = {
    click_cell: 'clickCell',
    dblclick_cell: 'dblclickCell',
    contextmenu_cell: 'rightClickRow',
    mouseenter_table: 'mouseenterTable',
    mouseleave_table: 'mouseleaveTable',
    mouseenter_cell: 'mouseenterCell',
    mouseleave_cell: 'mouseleaveCell',
    mousemove_cell: 'mousemoveCell',
    mousedown_cell: 'mousedownCell',
    mouseup_cell: 'mouseupCell',
    keydown: 'keydown',
    scroll: 'scroll',
    checkbox_state_change: 'checkboxStateChange',
    resize_column: 'resizeColumn',
    resize_column_end: 'resizeColumnEnd',
    change_header_position: 'changeHeaderPosition',
  };

  Object.entries(eventMap).forEach(([event, emitEvent]) => {
    listTable?.on(
      event as keyof VTable.TYPES.TableEventHandlersEventArgumentMap,
      (e) => {
        app && app.emit(emitEvent, e);
      },
    );
  });

  if (props.event) {
    Object.keys(props.event).forEach((key) => {
      listTable?.on(
        key as keyof typeof props.event,
        props.event[key] as unknown as VTable.TYPES.TableEventListener<
          keyof typeof props.event
        >,
      );
    });
  }
};
</script>

<style></style>
