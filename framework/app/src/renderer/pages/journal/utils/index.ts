import { dealKfTime } from '@kungfu-trader/kungfu-js-api/kungfu';
import { parseURIParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { getKfLocationByProcessId } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';

export const getSessionLocationById = (
  sessions: KungfuApi.Session[],
  uid: number,
): KungfuApi.Session | KungfuApi.KfLocation | null => {
  const sessionResolved = sessions.filter((item) => item.location_uid === uid);
  if (sessionResolved.length) {
    return sessionResolved[0];
  }

  return null;
};

export const dealFrameMsgType = (msgType: number) => msgType.toString();

export const dealFrameSourceToDest = (
  frameData: KungfuApi.Frame,
  sessions: KungfuApi.Session[],
) => {
  const sourceResolved = getSessionLocationById(
    sessions,
    frameData.source,
  )?.name;

  const destResolved = getSessionLocationById(sessions, frameData.dest)?.name;

  return `${sourceResolved} → ${destResolved}`;
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
  sessions: KungfuApi.SessionResolved[],
): KungfuApi.FrameResolved => {
  return {
    ...frame,
    gen_time_resolved: dealKfTime(frame.genTime, true),
    trigger_time_resolved: dealKfTime(frame.triggerTime, true),
    msg_type_resolved: dealFrameMsgType(frame.msgType),
    source_to_dest: dealFrameSourceToDest(frame, sessions),
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
