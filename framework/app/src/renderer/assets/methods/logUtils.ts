import path from 'path';
import os from 'os';
import { computed, reactive, Ref, ref, watch, nextTick } from 'vue';
import {
  debounce,
  isCriticalLog,
  KfFixedList,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { Tail } from 'tail';
import { messagePrompt, parseURIParams } from './uiUtils';
import { ensureFileSync } from 'fs-extra';

const { error } = messagePrompt();

export const getLogPath = (): string => {
  return path.resolve(decodeURI(parseURIParams().logPath) || '');
};

export const getUrlParams = (): Record<string, string> => {
  const urlParams = parseURIParams();
  const params: Record<string, string> = {};
  Object.keys(urlParams).forEach((key) => {
    if (key === 'logPath') {
      params[key] = decodeURI(urlParams[key]);
    } else {
      params[key] = urlParams[key];
    }
  });
  console.log('getUrlParams', params);
  return params;
};

export function preDealLogMessage(line: string): string {
  // 21 = pm2 timestamp length
  if (line.indexOf('[') === 21) {
    line = line.slice(21);
  }
  line = line.replace(/&/g, '&amp;');
  line = line.replace(/</g, '&lt;');
  line = line.replace(/>/g, '&gt;');
  line = line.replace(/"/g, '&quot;');
  line = line.replace(/'/g, '&#39;');
  line = line.replace(/`/g, '&#96;');
  line = line.replace(/\//g, '&#x2F;');
  return line;
}

export function dealLogMessage(line: string): string {
  if (isCriticalLog(line)) {
    line = `<span class="critical">${line}</span>`;
    return line;
  }

  line = line
    .replace(
      /(?<=\[)\s*(info|KF_INFO)\s*(?=\])/g,
      '<span class="info"> $1 </span>',
    )
    .replace(
      /(?<=\[)\s*(warning|KF_WARN)\s*(?=\])/g,
      '<span class="warning"> $1 </span>',
    )
    .replace(
      /(?<=\[)\s*(error|KF_ERROR)\s*(?=\])/g,
      '<span class="error"> $1 </span>',
    )
    .replace(/(?<=\[)\s*debug\s*(?=\])/g, '<span class="debug"> debug </span>')
    .replace(/(?<=\[)\s*trace\s*(?=\])/g, '<span class="trace"> trace </span>');

  return line;
}

export const useLogInit = (
  logPath: string,
  nLines = 10000,
): {
  logList: KungfuApi.KfFixedList<KungfuApi.KfLogData>;
  scrollToBottomChecked: Ref<boolean>;
  scrollerTableRef: Ref;
  isLoading: Ref<boolean>;
  scrollToBottom: () => void;
  startTailLog: () => void;
  clearLogState: () => void;
} => {
  // const LOADING_TIMEOUT = 2000;
  let LogTail: Tail | null = null;
  const logList = reactive<KungfuApi.KfFixedList<KungfuApi.KfLogData>>(
    new KfFixedList(nLines),
  );
  const scrollerTableRef = ref();
  const scrollToBottomChecked = ref<boolean>(false);
  const isLoading = ref<boolean>(false);

  ensureFileSync(logPath);

  const scrollToBottom = () => {
    if (scrollToBottomChecked.value) {
      scrollerTableRef.value.scrollToBottom();
    }
  };

  const startTailLog = () => {
    let lastLineReceivedAt = Date.now();

    isLoading.value = true;
    LogTail && LogTail.unwatch();
    LogTail = new Tail(logPath, {
      follow: true,
      nLines: nLines,
      useWatchFile: os.platform() === 'win32',
    });

    let markId: number = +new Date();
    LogTail.on('line', (line: string) => {
      lastLineReceivedAt = Date.now();
      logList.insert({
        id: markId++,
        message: dealLogMessage(preDealLogMessage(line)),
        messageOrigin: line,
        messageForSearch: '',
      });
      scrollToBottom();
    });

    LogTail.on('error', (err: Error) => {
      error(err.message);
    });

    LogTail.watch();
    const timeoutId = setInterval(checkLoadingStatus, 1000); // 每1秒检查一次

    function checkLoadingStatus() {
      if (Date.now() - lastLineReceivedAt > 1000) {
        // 1秒没有接收到新行
        isLoading.value = false;
        clearInterval(timeoutId);
      }
    }
  };

  const clearLogState = () => {
    logList.list = [];
    LogTail?.unwatch();
    LogTail = null;
    isLoading.value = false;
  };

  return {
    logList,
    scrollToBottomChecked,
    scrollerTableRef,
    isLoading,
    scrollToBottom,
    startTailLog,
    clearLogState,
  };
};

export const useLogSearch = (
  logList: KungfuApi.KfFixedList<KungfuApi.KfLogData>,
  scrollerTableRef: Ref,
  contentSize: Ref<{
    width: number;
    height: number;
  }>,
): {
  inputSearchRef: Ref;
  searchKeyword: Ref<string>;
  currentResultPointerIndex: Ref<number>;
  totalResultCount: Ref<number>;
  clearLogSearchState: () => void;
  handleToDownSearchResult: () => void;
  handleToUpSearchResult: () => void;
} => {
  const escapeRegExp = (string: string) => {
    return string
      .replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
      .replace(/\s+/g, '\\s+')
      .replace(/\//g, '(\\/|&#x2F;)')
      .replace(/&/g, '(&|&amp;)')
      .replace(/</g, '(<|&lt;)')
      .replace(/>/g, '(>|&gt;)')
      .replace(/"/g, '("|&quot;)')
      .replace(/'/g, "('|&#39;)")
      .replace(/`/g, '(`|&#96;)');
  };
  const inputSearchRef = ref();
  const searchKeyword = ref<string>('');
  const searchKeywordReg = computed(() => {
    let reg: RegExp | null = null;
    try {
      reg = new RegExp(escapeRegExp(searchKeyword.value), 'g');
    } catch (err) {
      console.error(err);
    }

    return reg;
  });

  const currentResultPointerIndex = ref<number>(0);
  const totalResultCount = ref<number>(0);
  const searchLogResults: KungfuApi.KfLogData[] = [];

  const clearLogSearchState = (): void => {
    logList.list.forEach((logData: KungfuApi.KfLogData) => {
      logData.messageForSearch = '';
    });
    searchLogResults.length = 0;
    currentResultPointerIndex.value = 0;
    totalResultCount.value = 0;
    searchKeyword.value = '';
  };

  const setLogDataMessageForSearch = (
    item: KungfuApi.KfLogData,
    currentPointer = false,
  ): string => {
    if (searchKeywordReg.value === null) return '';
    if (currentPointer) {
      return dealLogMessage(
        preDealLogMessage(item.messageOrigin).replace(
          searchKeywordReg.value,
          `<font class="search-keyword current-search-pointer">${searchKeyword.value}</font>`,
        ),
      );
    } else {
      return dealLogMessage(
        preDealLogMessage(item.messageOrigin).replace(
          searchKeywordReg.value,
          `<font class="search-keyword">${searchKeyword.value}</font>`,
        ),
      );
    }
  };

  const isLogItemVisiable = (
    logId: number,
    contentSize: Ref<{ width: number; height: number }>,
  ): boolean => {
    const $items: NodeList = document.querySelectorAll(`#kf-log-item-${logId}`);
    const logContentOffsetY = 32 + 8;
    const logContentHeight = contentSize.value.height - 16;

    if ($items.length) {
      const $item = ([...$items] as HTMLElement[]).filter((item) => {
        if (item) {
          return item.getAttribute('active') === 'true';
        }

        return false;
      });

      if ($item.length) {
        const $itemResolved = $item[0];
        const rect = $itemResolved.getBoundingClientRect();
        if (rect.top > logContentOffsetY && rect.top < logContentHeight) {
          return true;
        }
      }
    }

    return false;
  };

  const srollToItemByIndexInLogList = (logId: number): void => {
    if (isLogItemVisiable(logId, contentSize)) {
      return;
    }

    const index = logList.list.findIndex((item) => item.id === logId);
    if (index >= 0) {
      scrollerTableRef.value.scrollToItem(index);
    }
  };

  const initSearchPointerIndex = (): void => {
    const total = searchLogResults.length;
    totalResultCount.value = total;
    if (total) {
      for (let i = 0; i < total; i++) {
        const id = searchLogResults[i].id;
        if (isLogItemVisiable(id, contentSize)) {
          const index = searchLogResults.findIndex((item) => item.id === id);
          currentResultPointerIndex.value = index < 0 ? 0 : index + 1;
          srollToItemByIndexInLogList(id);
          return;
        }
      }

      currentResultPointerIndex.value = 1;
    } else {
      currentResultPointerIndex.value = 0;
    }
  };

  watch(
    searchKeywordReg,
    debounce(() => {
      //clean
      if (
        searchKeyword.value.trim() === '' ||
        searchKeywordReg.value === null
      ) {
        clearLogSearchState();
        return;
      }

      searchLogResults.length = 0;
      currentResultPointerIndex.value = 0;
      totalResultCount.value = 0;
      nextTick().then(() => {
        logList.list.forEach((item: KungfuApi.KfLogData) => {
          const match = item.messageOrigin.match(/(\[.*)/);
          if (
            searchKeywordReg.value &&
            searchKeywordReg.value.test(match ? match[1] : item.messageOrigin)
          ) {
            item.messageForSearch = setLogDataMessageForSearch(item);
            searchLogResults.push(item);
          } else {
            item.messageForSearch = '';
          }
        });

        initSearchPointerIndex();
      });
    }),
  );

  watch(currentResultPointerIndex, (newIndex: number, oldIndex: number) => {
    if (newIndex === 0) {
      return;
    }

    const oldId = oldIndex ? searchLogResults[oldIndex - 1].id : 0;
    const newId = searchLogResults[newIndex - 1].id;

    logList.list.forEach((logData: KungfuApi.KfLogData) => {
      if (logData.id === oldId && oldId !== 0) {
        logData.messageForSearch = setLogDataMessageForSearch(logData);
      }

      if (logData.id === newId) {
        logData.messageForSearch = setLogDataMessageForSearch(logData, true);

        srollToItemByIndexInLogList(newId);
      }
    });
  });

  const handleToDownSearchResult = (): void => {
    if (totalResultCount.value === 0) return;
    if (currentResultPointerIndex.value === totalResultCount.value) {
      currentResultPointerIndex.value = 1;
    } else {
      currentResultPointerIndex.value++;
    }
  };

  const handleToUpSearchResult = (): void => {
    if (totalResultCount.value === 0) return;
    if (currentResultPointerIndex.value === 1) {
      currentResultPointerIndex.value = totalResultCount.value;
    } else {
      currentResultPointerIndex.value--;
    }
  };

  return {
    inputSearchRef,
    searchKeyword,
    currentResultPointerIndex,
    totalResultCount,
    clearLogSearchState,
    handleToDownSearchResult,
    handleToUpSearchResult,
  };
};
