import fse from 'fs-extra';
import path from 'path';
import { format } from '@fast-csv/format';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { parseURIParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  getIdByKfLocation,
  getKfLocationByProcessId,
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
  JournalFrameMsgType[+msgType] || { name: '', color: 'default' };

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

export const dealFrame = (
  frame: KungfuApi.Frame,
  sessionMap: Record<number, KungfuApi.KfLocation>,
): KungfuApi.FrameResolved => {
  const destResolved = dealDestOrSource('dest', frame, sessionMap);
  const sourceResolved = dealDestOrSource('source', frame, sessionMap);
  return {
    ...frame,
    genTimeResolved: dealKfTime(frame.genTime, true),
    triggerTimeResolved: dealKfTime(frame.triggerTime, true),
    msgTypeResolved: dealFrameMsgType(frame.msgType),
    destResolved,
    sourceResolved,
    sourceToDest: dealFrameSourceToDest(frame.sourceName, frame.destName),
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
