import path from 'path';
import os from 'os';
import { reactive, Ref, ref } from 'vue';
import {
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
    return line.slice(21);
  }
  return line;
}

export function dealLogMessage(line: string): string {
  if (isCriticalLog(line)) {
    line = `<span class="critical">${line}</span>`;
    return line;
  }

  const createLogLevelRegExp = (level: string) =>
    new RegExp(`(?<=\\[(<\\/mark>)?)\\s*(${level})\\s*(?=(<mark .*>)?\\])`);

  line = line
    .replace(
      createLogLevelRegExp('info|KF_INFO'),
      '<span class="info"> $2 </span>',
    )
    .replace(
      createLogLevelRegExp('warning|KF_WARN'),
      '<span class="warning"> $2 </span>',
    )
    .replace(
      createLogLevelRegExp('error|KF_ERROR'),
      '<span class="error"> $2 </span>',
    )
    .replace(
      createLogLevelRegExp('debug'),
      '<span class="debug"> debug </span>',
    )
    .replace(
      createLogLevelRegExp('trace'),
      '<span class="trace"> trace </span>',
    );

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
        message: preDealLogMessage(line),
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
