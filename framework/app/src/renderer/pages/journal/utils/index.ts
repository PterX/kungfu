import fse from 'fs-extra';
import path from 'path';
import { format } from '@fast-csv/format';
import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { parseURIParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  getIdByKfLocation,
  getKfLocationByProcessId,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';

export const getSessionLocationById = (
  sessionMap: Record<number, KungfuApi.KfLocation>,
  uid: number,
): KungfuApi.KfLocation | null => {
  if (!sessionMap[uid]) return null;
  return sessionMap[uid];
};

export const dealFrameMsgType = (msgType: number) => msgType.toString();

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
  dest_resolved: string,
  source_resolved: string,
) => {
  return `${dest_resolved} → ${source_resolved}`;
};

const dealFrameData = (data: string): unknown[] => {
  try {
    const object = JSON.parse(data);
    const formatToTreeData = (obj: unknown) => {
      if (typeof obj !== 'object' || obj === null) return [];
      if (!Object.keys(obj).length) return [];

      const dealValue = (value) => {
        if (typeof value === 'boolean' || !Number.isNaN(+value)) return value;

        return `"${value}"`;
      };

      return Object.keys(obj).map((key) => {
        const children = formatToTreeData(obj[key]);
        return {
          title: children?.length ? key : `"${key}" : ${dealValue(obj[key])}`,
          key,
          ...(children?.length ? { children } : {}),
        };
      });
    };

    return [
      { title: '{', key: 'root-start', children: formatToTreeData(object) },
      { title: '}', key: 'root-end' },
    ];
  } catch (error) {
    return [{ title: 'null', key: 'root' }];
  }
};

export const dealFrame = (
  frame: KungfuApi.Frame,
  sessionMap: Record<number, KungfuApi.KfLocation>,
): KungfuApi.FrameResolved => {
  const dest_resolved = dealDestOrSource('dest', frame, sessionMap);
  const source_resolved = dealDestOrSource('source', frame, sessionMap);
  return {
    ...frame,
    gen_time_resolved: dealKfTime(frame.genTime, true),
    trigger_time_resolved: dealKfTime(frame.triggerTime, true),
    msg_type_resolved: dealFrameMsgType(frame.msgType),
    dest_resolved,
    source_resolved,
    source_to_dest: dealFrameSourceToDest(dest_resolved, source_resolved),
    data_resolved: dealFrameData(frame.data),
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
        stream.write(headers);

        for (const i of data) {
          stream.write(headers.map((header) => i[header]));
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
