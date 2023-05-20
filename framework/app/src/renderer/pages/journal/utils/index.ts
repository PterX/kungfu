import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import fse from 'fs-extra';
import path from 'path';
import { format } from '@fast-csv/format';
import { dealKfTime, longfist } from '@kungfu-trader/kungfu-js-api/kungfu';
import { parseURIParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import {
  getIdByKfLocation,
  getKfLocationByProcessId,
  getProcessIdByKfLocation,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { KfCategory } from '@kungfu-trader/kungfu-js-api/config/tradingConfig';
import {
  KfCategoryEnum,
  FrameMsgTypeEnum,
  KfCategoryTypes,
} from '@kungfu-trader/kungfu-js-api/typings/enums';

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
  return {
    ...session,
    session_id_resolved: getProcessIdByKfLocation(session),
    begin_time_resolved: dealKfTime(getAbs<bigint>(session.begin_time)),
    end_time_resolved: dealKfTime(getAbs<bigint>(session.end_time)),
    status: getSessionStatus(session),
    session_id_origin: `${getProcessIdByKfLocation(session)}_${session.index}`,
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
  msgType: FrameMsgTypeEnum,
): KungfuApi.KfTradeValueCommonData => {
  const msgTypeId = Number(msgType);
  if (msgTypeId > MSG_NUM) {
    return getMsgResolved(msgTypeId);
  } else {
    return getMsgResolved(msgTypeId % MSG_NUM);
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
    sourceToDest: getSourceToDest(
      source,
      dest,
      session.location_uid,
      locationNameMap,
    ),
    msgTypeName: longfist.msgTypes[+frame.msgType],
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

export const getSourceToDest = (
  source: number,
  dest: number,
  currentLocationUid: number,
  locationMap: Record<string, string>,
): string => {
  const sourceLocationName = locationMap[source + ''];
  const destLocationName = locationMap[dest + ''];
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
    MsgType: frame.msgTypeName || frame.msgType,
    PageId: frame.pageId,
    FrameId: frame.frameId,
    SourceToDest: frame.sourceToDest || `${frame.source} -> ${frame.dest}`,
  };
};
