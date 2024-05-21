import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import fse from 'fs-extra';
import path from 'path';
import { format } from '@fast-csv/format';
import { dealKfTime, io, longfist } from '@kungfu-trader/kungfu-js-api/kungfu';
import { parseURIParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  getIdByKfLocation,
  deepClone,
  getKfLocationByProcessId,
} from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { KfCategory } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import {
  KfCategoryEnum,
  KfModeEnum,
  KfCategoryTypes,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import {
  getCurrentInstance,
  nextTick,
  onBeforeUnmount,
  onMounted,
  ref,
} from 'vue';
import { filter } from 'rxjs';

const MSG_NUM = 10000;

export const getAbs = <T extends number | bigint>(num: T): T =>
  num < 0 ? (-num as T) : num;

export const getSessionStatus = (session: KungfuApi.Session) =>
  session.end_time != 0n
    ? SessionStatusEnum.Finished
    : SessionStatusEnum.Running;

export const dealSession = (
  session: KungfuApi.Session,
): KungfuApi.SessionResolved => {
  session.category = KfCategoryEnum[
    session.category as KfCategoryEnum
  ] as KfCategoryTypes;
  session.mode = KfModeEnum[session.mode as KfModeEnum];
  return {
    ...session,
    sessionName: getIdByKfLocation(session),
    beginTimeResolved: dealKfTime(getAbs<bigint>(session.begin_time)),
    endTimeResolved: dealKfTime(getAbs<bigint>(session.end_time)),
    status: getSessionStatus(session),
  };
};

export const dealSessionsToMap = (sessions: KungfuApi.Session[]) => {
  return sessions.reduce((sessionsMap, cur) => {
    sessionsMap[`${cur.begin_time}`] = dealSession(cur);
    return sessionsMap;
  }, {} as Record<string, KungfuApi.SessionResolved>);
};

export const getSessionLocationById = (
  sessionMap: Record<number, KungfuApi.KfLocation>,
  uid: number,
): KungfuApi.KfLocation | null => {
  if (!sessionMap[uid]) return null;
  return sessionMap[uid];
};

export const dealCategory = (
  category: KfCategoryTypes,
): KungfuApi.KfTradeValueCommonData => {
  return KfCategory[KfCategoryEnum[category]];
};

export const getMsgResolved = (
  num: number,
): KungfuApi.KfTradeValueCommonData => {
  if (num > 100 && num < 200) {
    return {
      name: '账户信息',
      color: 'blue',
    };
  } else if (num > 200 && num < 300) {
    return {
      name: '交易相关',
      color: '#FAAD14',
    };
  } else if (num > 300 && num < 400) {
    return {
      name: '查询相关',
      color: 'default',
    };
  } else if (num > 400 && num < 500) {
    return {
      name: '行情相关',
      color: 'green',
    };
  } else if (num > 500 && num < 600) {
    return {
      name: '行情订阅相关',
      color: 'purple',
    };
  } else if (num > 600 && num < 700) {
    return {
      name: '算子相关',
      color: 'default',
    };
  } else {
    return {
      name: '其他',
      color: 'default',
    };
  }
};

export const dealFrameMsgType = (
  msgType: number,
): KungfuApi.KfTradeValueCommonData => {
  if (msgType > MSG_NUM) {
    return getMsgResolved(msgType);
  } else {
    return getMsgResolved(msgType % MSG_NUM);
  }
};

export const dealDestOrSource = (
  type: 'source' | 'dest',
  frame: KungfuApi.Frame,
  sessionMap: Record<number, KungfuApi.KfLocation>,
) => {
  const locationResolved = getSessionLocationById(sessionMap, frame[type]);
  const locationId = locationResolved
    ? getIdByKfLocation(locationResolved as KungfuApi.KfLocation)
    : frame[type];

  return locationId + '';
};

export const dealFrame = (
  frame: KungfuApi.Frame,
  session: KungfuApi.SessionResolved,
  locationNameMap: Record<string, string>,
): KungfuApi.FrameResolved => {
  const { source, dest, pageId, frameId } = frame;
  return {
    ...frame,
    id: `${source}_${dest}_${pageId}_${frameId}`,
    genTimeResolved: dealKfTime(frame.genTime, true),
    triggerTimeResolved: dealKfTime(frame.triggerTime, true),
    msgTypeResolved: dealFrameMsgType(frame.msgType),
    initialSourceResolved:
      locationNameMap[frame.initialSource + ''] || frame.initialSource + '',
    sourceToDest: getSourceToDest(
      source,
      dest,
      session.location_uid,
      locationNameMap,
    ),
    msgTypeName: longfist.msgTypes[+frame.msgType] || frame.msgType + '',
  };
};

export const getCurrentLocation = (): KungfuApi.KfExtractLocation | null => {
  const location = getKfLocationByProcessId(
    decodeURI(parseURIParams().processId) || '',
  );
  const uid = +(decodeURI(parseURIParams().locationUID) || '');
  if (!location || !uid) {
    return null;
  }

  return {
    ...location,
    uid,
    uname: '',
  };
};

export const writeCsvByStream = <T>(
  filePath: string,
  data: T[],
  headers?: string[],
  headerTransform = (headerItem: string) => headerItem,
  dataTransform = (dataItem, headerItem: string) => {
    headerItem;
    return dataItem;
  },
) => {
  return new Promise((resolve, reject) => {
    filePath = path.normalize(filePath);

    const stream = format();
    const fileWriteStream = fse.createWriteStream(filePath);
    fileWriteStream.write(Buffer.from('\xEF\xBB\xBF', 'binary'));
    fileWriteStream.on('error', (error) => {
      reject(error);
    });
    stream.pipe(fileWriteStream);
    if (typeof data[0] === 'object') {
      const isFirstStringArray = Array.isArray(data[0])
        ? data[0].every((item) => typeof item === 'string')
        : false;
      if (!data.length) reject('empty_data');

      if (!headers) {
        if (isFirstStringArray) {
          headers = data[0] as unknown as string[];
        } else {
          return reject(
            new Error('Set the correct headers or in the first item of data'),
          );
        }
      }

      try {
        stream.write(headers.map((item) => headerTransform(item)));

        for (const i of data) {
          stream.write(
            headers.map((header) => dataTransform(i[header], header)),
          );
        }
      } catch (error) {
        reject(error);
      }
    } else {
      stream.end();
      return reject(new Error('The data with the wrong format.'));
    }

    stream.end(() => {
      resolve(true);
    });
  });
};

export const getSourceToDest = (
  source: number,
  dest: number,
  currentLocationUid: number,
  locationMap: Record<string, string>,
): string => {
  const sourceLocationName = locationMap[source + ''] || source;
  const destLocationName = locationMap[dest + ''] || dest;
  if (source === currentLocationUid) {
    return `self -> ${destLocationName}`;
  } else if (dest === currentLocationUid) {
    return `${sourceLocationName} -> self`;
  } else {
    return `${sourceLocationName} -> ${destLocationName}`;
  }
};

export interface FrameHeaderForShow {
  DataLength: number;
  GenTime: string;
  TriggerTime: string;
  InitialSourceResolved: string;
  MsgType: string;
  PageId: number;
  FrameId: number;
  SourceToDest: string;
}

export const buildFrameHeaderForShow = (
  frame: KungfuApi.FrameResolved,
): FrameHeaderForShow => {
  return {
    DataLength: frame.dataLength,
    GenTime: frame.genTimeResolved,
    TriggerTime: frame.triggerTimeResolved,
    MsgType: frame.msgTypeName,
    InitialSourceResolved: frame.initialSourceResolved,
    PageId: frame.pageId,
    FrameId: frame.frameId,
    SourceToDest: frame.sourceToDest || `${frame.source} -> ${frame.dest}`,
  };
};

export const msgTypes = deepClone(longfist.msgTypes);
export const MsgTypes = ((): Record<string, number> => {
  return Object.keys(msgTypes).reduce((acc, key) => {
    acc[msgTypes[key]] = +key;
    return acc;
  }, {} as Record<string, number>);
})();

export const useResizeFlag = () => {
  const app = getCurrentInstance();
  const contentVisible = ref<boolean>(true);
  onMounted(() => {
    if (app?.proxy) {
      const subscription = app?.proxy.$globalBus
        .pipe(filter((e: KfEvent.KfBusEvent) => e.tag === 'resize'))
        .subscribe(async () => {
          contentVisible.value = false;
          await nextTick();
          contentVisible.value = true;
        });

      onBeforeUnmount(() => {
        subscription.unsubscribe();
      });
    }
  });

  return {
    contentVisible,
  };
};

export const resolveLocations = (
  obj: Record<string, KungfuApi.KfExtractLocation>,
) => {
  const output: Record<string, string> = {};

  for (const key in obj) {
    // eslint-disable-next-line no-prototype-builtins
    if (obj.hasOwnProperty(key)) {
      const item = obj[key];
      output[key] = `${item.category}/${item.group}/${item.name}/${item.mode}`;
    }
  }
  output['0'] = 'public';
  output['1'] = 'sync';

  return output;
};

export const getSourceDestMap = () => {
  const locations = Object.values(io.getAllLocations());
  const locationsMap = locations.reduce((pre, cur) => {
    pre[cur.uid] = cur;
    return pre;
  }, {} as Record<string, KungfuApi.KfExtractLocation>);
  return resolveLocations(locationsMap);
};

export const useNow = () => {
  const now = ref(getNowInNano());
  let timer: NodeJS.Timeout;

  const updateNowTime = () => {
    now.value = getNowInNano();
    clearTimeout(timer);
    timer = setTimeout(updateNowTime, 1000);
  };
  onMounted(() => {
    updateNowTime();
  });
  return {
    now,
  };
};

export const getNowInNano = () => BigInt(new Date().getTime()) * 1000000n;
