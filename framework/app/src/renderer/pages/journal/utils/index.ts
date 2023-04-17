import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { WorkerReceiver } from './../workers/receiver';
import { storeToRefs } from 'pinia';
import { useJournalStore } from './../store/journalStore';
import { WorkerSender } from './../workers/sender';
import { watch, reactive, computed } from 'vue';
import fse from 'fs-extra';
import path from 'path';
import { format } from '@fast-csv/format';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { parseURIParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  getIdByKfLocation,
  getKfLocationByProcessId,
  getProcessIdByKfLocation,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import {
  JournalFrameMsgType,
  KfCategory,
} from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import {
  KfCategoryEnum,
  FrameMsgTypeEnum,
  KfCategoryTypes,
} from '@kungfu-trader/kungfu-js-api/typings/enums';

const consoleError = (error, ...datas) => {
  console.log(...(datas.length ? ['datas: ', ...datas, '\n', error] : error));
};

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
  return {
    ...session,
    session_id_resolved: getProcessIdByKfLocation(session),
    begin_time_resolved: dealKfTime(getAbs<bigint>(session.begin_time)),
    end_time_resolved: dealKfTime(getAbs<bigint>(session.end_time)),
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

export const dealFrameMsgType = (
  msgType: FrameMsgTypeEnum,
): KungfuApi.KfTradeValueCommonData =>
  JournalFrameMsgType[+msgType] || { name: msgType, color: 'default' };

export const dealDestOrSource = (
  type: 'source' | 'dest',
  frameData: KungfuApi.Frame,
  sessionMap: Record<number, KungfuApi.KfLocation>,
) => {
  const locationResolved = getSessionLocationById(sessionMap, frameData[type]);
  const locationId = locationResolved
    ? getIdByKfLocation(locationResolved as KungfuApi.KfLocation)
    : frameData[type];

  return locationId + '';
};

export const dealFrameSourceToDest = (
  sourceResolved: string,
  destResolved: string,
) => {
  return `${sourceResolved} → ${destResolved}`;
};

const dealFrameData = (data: string): unknown[] => {
  try {
    const object = JSON.parse(data);
    const formatToTreeData = (obj: unknown) => {
      if (typeof obj === 'string') {
        if (obj.indexOf('{') !== -1 || obj.indexOf('[') !== -1) {
          try {
            obj = JSON.parse(obj);
          } catch (error) {
            consoleError(error, obj);
          }
        }
      }

      if (typeof obj !== 'object' || obj === null) return [];
      if (!Object.keys(obj).length) return [];

      const dealKeyOrValue = (value) => {
        if (typeof value === 'boolean' || !Number.isNaN(+value)) return value;

        return `"${value}"`;
      };

      const obj1 = obj; // to fix ts error

      return Object.keys(obj).map((key) => {
        const children = formatToTreeData(obj1[key]);
        return {
          title: children?.length
            ? `${key} : ${Array.isArray(obj1[key]) ? '[' : '{'}`
            : `${dealKeyOrValue(key)} : ${dealKeyOrValue(obj1[key])},`,
          key,
          ...(children?.length
            ? {
                children: [
                  ...children,
                  { title: Array.isArray(obj1[key]) ? ']' : '}' },
                ],
              }
            : {}),
        };
      });
    };

    return [
      { title: '{', key: 'root-start', children: formatToTreeData(object) },
      { title: '}', key: 'root-end' },
    ];
  } catch (error) {
    consoleError(error, data);
    return [{ title: 'null', key: 'root' }];
  }
};

export const dealFrame = (frame: KungfuApi.Frame): KungfuApi.FrameResolved => {
  return {
    ...frame,
    genTimeResolved: dealKfTime(frame.genTime, true),
    triggerTimeResolved: dealKfTime(frame.triggerTime, true),
    msgTypeResolved: dealFrameMsgType(frame.msgType),
    destResolved: 'TODO',
    sourceResolved: 'TODO',
    // sourceToDest: dealFrameSourceToDest(frame.sourceName, frame.destName),
    sourceToDest: 'TODO',
    dataResolved: dealFrameData(frame.data),
  };
};

export const getCurrentLocation = () => {
  const location = getKfLocationByProcessId(
    decodeURI(parseURIParams().processId) || '',
  );
  const location_uid = +(decodeURI(parseURIParams().locationUid) || '');

  if (!location || !location_uid) {
    return null;
  }

  return {
    ...location,
    location_uid,
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
    stream.pipe(fse.createWriteStream(filePath));

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

export const useDealJournalDatas = () => {
  type DataWrapper<T> = {
    data: Record<string, T[]>;
    isInit: boolean;
  };

  const quotes = reactive<DataWrapper<KungfuApi.Quote>>({
    data: {},
    isInit: true,
  });
  const orders = reactive<DataWrapper<KungfuApi.Order>>({
    data: {},
    isInit: true,
  });
  const trades = reactive<DataWrapper<KungfuApi.Trade>>({
    data: {},
    isInit: true,
  });

  const journalStore = useJournalStore();
  const journalState = storeToRefs(journalStore);

  const worker = window.workers.dealJournalDatas;
  const dataSender = new WorkerSender<KungfuApi.FrameResolved>(worker, 200);
  const dataReceiver = new WorkerReceiver('send', worker);

  watch(
    () => journalState.lastUpdateSessionFrames.value,
    (frames) => {
      dataSender.sendData('send-events', frames, {
        isInit: journalState.isSessionFramesInit.value,
      });
    },
  );

  const groupDataByInstrAndExcId = <
    T extends KungfuApi.Trade | KungfuApi.Quote | KungfuApi.Order,
  >(
    data: T[],
  ): Record<string, T[]> => {
    return data.reduce((data, cur) => {
      const key = `${cur.exchange_id}_${cur.instrument_id}`;
      if (key in data && Array.isArray(data[key])) {
        data[key].push(cur);
      } else {
        data[key] = [cur];
      }
      return data;
    }, {} as Record<string, T[]>);
  };

  const dealUpdateData = <
    T extends KungfuApi.Trade | KungfuApi.Quote | KungfuApi.Order,
  >(
    exsitedData: Record<string, T[]>,
    curData: T[],
    isInit: boolean,
  ) => {
    const resolvedCurData = groupDataByInstrAndExcId(curData);
    if (isInit) {
      Object.keys(resolvedCurData).forEach((key) => {
        exsitedData[key] = resolvedCurData[key];
      });
    } else {
      Object.keys(resolvedCurData).forEach((key) => {
        exsitedData[key].push(...resolvedCurData[key]);
      });
    }
  };

  dataReceiver.onEnd<KungfuApi.Quote>('send-quotes', ({ data, info }) => {
    console.log('quote', data, info);
    dealUpdateData(quotes.data, data, info?.isInit);
    quotes.isInit = info?.isInit;
  });

  dataReceiver.onEnd<KungfuApi.Trade>('send-trades', ({ data, info }) => {
    console.log('trade', data, info);
    dealUpdateData(trades.data, data, info?.isInit);
    trades.isInit = info?.isInit;
  });

  dataReceiver.onEnd<KungfuApi.Order>('send-orders', ({ data, info }) => {
    console.log('order', data, info);
    dealUpdateData(orders.data, data, info?.isInit);
    orders.isInit = info?.isInit;
  });

  const allTradingDatas = computed(() => {
    const keys = Array.from(
      new Set([
        ...Object.keys(quotes.data),
        ...Object.keys(orders.data),
        ...Object.keys(trades.data),
      ]),
    );

    return keys.reduce(
      (datas, key) => {
        return {
          ...datas,
          [key]: {
            quotes: quotes.data[key] ?? [],
            trades: trades.data[key] ?? [],
            orders: orders.data[key] ?? [],
          },
        };
      },
      {} as Record<
        string,
        {
          quotes: KungfuApi.Quote[];
          orders: KungfuApi.Order[];
          trades: KungfuApi.Trade[];
        }
      >,
    );
  });

  return {
    quotes,
    orders,
    trades,
    allTradingDatas,
  };
};
