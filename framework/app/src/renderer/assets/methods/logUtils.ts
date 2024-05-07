import path from 'path';
import os from 'os';
import { reactive, Ref, ref } from 'vue';
import {
  isCriticalLog,
  KfFixedList,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
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
      params[key] = decodeURI(urlParams[key]).replace('%20', ' ');
    } else {
      params[key] = urlParams[key].replace('%20', ' ');
    }
  });
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
  nLines = 10000,
): {
  logList: KungfuApi.KfFixedList<KungfuApi.KfLogData>;
  scrollToBottomChecked: Ref<boolean>;
  scrollerTableRef: Ref;
  isLoading: Ref<boolean>;
  scrollToBottom: () => void;
  startTailLog: (logPath: string) => void;
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
  let loadingTimeoutId: NodeJS.Timeout | null = null;
  let insertTimerId: NodeJS.Timeout | null = null;
  const INSERT_LINES = 1000;

  const scrollToBottom = () => {
    if (scrollToBottomChecked.value) {
      scrollerTableRef.value.scrollToBottom();
    }
  };

  const updateLoading = () => {
    isLoading.value = true;
    if (loadingTimeoutId) {
      clearTimeout(loadingTimeoutId);
    }
    loadingTimeoutId = setTimeout(() => {
      isLoading.value = false;
    }, 1000);
  };

  const startTailLog = (logPath: string) => {
    ensureFileSync(logPath);

    LogTail && LogTail.unwatch();
    LogTail = new Tail(logPath, {
      follow: true,
      nLines: nLines,
      useWatchFile: os.platform() === 'win32',
    });

    let markId: number = Date.now();
    const remineLogQueue: KungfuApi.KfLogData[] = [];

    LogTail.on('line', (line: string) => {
      const logData = { id: ++markId, message: preDealLogMessage(line) };
      if (logList.list.length < nLines) {
        updateLoading();
        logList.insert(logData);
        scrollToBottom();
      } else {
        remineLogQueue.push(logData);
      }
    });

    insertTimerId = setInterval(() => {
      if (logList.list.length < nLines) {
        return;
      }
      const newList = remineLogQueue.splice(0, INSERT_LINES);
      const length = newList.length;
      if (length > 0) {
        updateLoading();
        logList.list.splice(0, length);
        logList.list = logList.list.concat(newList);
        scrollToBottom();
      } else {
        return;
      }
    }, 200);

    LogTail.on('error', (err: Error) => {
      error(err.message);
    });

    LogTail.watch();
  };
  const clearLogState = () => {
    if (insertTimerId) {
      clearInterval(insertTimerId);
      insertTimerId = null;
    }
    if (loadingTimeoutId) {
      clearTimeout(loadingTimeoutId);
      loadingTimeoutId = null;
    }
    isLoading.value = false;
    logList.list = [];
    if (LogTail) {
      LogTail.unwatch();
      LogTail = null;
    }
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
