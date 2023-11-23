import fse from 'fs-extra';
import jschardet from 'jschardet';
import iconv from 'iconv-lite';
import {
  KF_INSTRUMENTS_PATH,
  KF_SUBSCRIBED_INSTRUMENTS_JSON_PATH,
} from '@kungfu-trader/kungfu-js-api/config/pathConfig';
import { InstrumentTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';

const defaultCharset = 'utf8';

const charsetTransformMap = {
  'UTF-8': 'utf8',
  GBK: 'gbk',
  GB2312: 'gb2312',
  GB18030: 'gb18030',
  Big5: 'big5',
};

function getBufferCharset(buffer: Buffer) {
  const { encoding, confidence } = jschardet.detect(buffer);

  if (confidence < 0.8) return defaultCharset;

  return encoding in charsetTransformMap
    ? charsetTransformMap[encoding]
    : defaultCharset;
}

function decodeBuffer(name: number[]) {
  name = name.filter((n) => !!n);
  const bufferFrom = Buffer.from(name as unknown as ArrayBuffer);
  const charset = getBufferCharset(bufferFrom);

  return iconv.decode(bufferFrom, charset);
}

type InstrumentResolvedData = Record<string, KungfuApi.InstrumentResolved>;

const resolveInstruments = (
  existedInstruments: InstrumentResolvedData,
  instruments: KungfuApi.Instrument[],
): InstrumentResolvedData => {
  return (instruments || []).reduce((existedData, item) => {
    const { instrument_id, instrument_type, product_id, exchange_id, ukey } =
      item;
    const oldInstrument = existedData[ukey] || null;
    const instrumentName = decodeBuffer(product_id).replaceAll("'", '');
    const instrumentNameResolved =
      instrumentName || oldInstrument?.instrumentName || '';
    const instrumentType =
      instrument_type ||
      oldInstrument?.instrumentType ||
      InstrumentTypeEnum.unknown;

    const newInstrument: KungfuApi.InstrumentResolved = {
      instrumentId: instrument_id,
      instrumentType,
      instrumentName: instrumentNameResolved,
      exchangeId: exchange_id,
      id: `${instrument_id}_${instrumentNameResolved}_${exchange_id}`.toLowerCase(),
      ukey,
    };
    existedData[ukey] = newInstrument;
    return existedData;
  }, existedInstruments);
};

const safeReadJsonSync = <T>(path: string, defaultContent: T): T => {
  try {
    return fse.readJSONSync(path);
  } catch (error) {
    return defaultContent;
  }
};

const resolveSubscribedInstruments = (
  existedInstruments: KungfuApi.InstrumentResolved[],
  instruments: InstrumentResolvedData,
) => {
  return existedInstruments.reduce((existData, item) => {
    if (instruments[item.ukey]) {
      existData.push(instruments[item.ukey]);
    } else {
      existData.push(item);
    }
    return existData;
  }, [] as KungfuApi.InstrumentResolved[]);
};

self.addEventListener('message', (e) => {
  const { instruments, tag } = e.data || {};

  if (tag === 'req_dealInstruments') {
    // instruments
    const existedInstruments: InstrumentResolvedData = safeReadJsonSync(
      KF_INSTRUMENTS_PATH,
      {} as InstrumentResolvedData,
    );
    const newInstruments: InstrumentResolvedData = resolveInstruments(
      existedInstruments,
      instruments,
    );

    if (!Object.keys(newInstruments || {}).length) {
      return;
    }

    fse.outputJSONSync(KF_INSTRUMENTS_PATH, newInstruments);

    self.postMessage({
      updateTime: new Date().getTime(),
      instruments: newInstruments,
    });

    // subscribed instruments
    const existedSubscribedInstruments: KungfuApi.InstrumentResolved[] =
      safeReadJsonSync(
        KF_SUBSCRIBED_INSTRUMENTS_JSON_PATH,
        [] as KungfuApi.InstrumentResolved[],
      );
    const newSubscribedInstruments: KungfuApi.InstrumentResolved[] =
      resolveSubscribedInstruments(
        existedSubscribedInstruments,
        newInstruments,
      );

    if (newSubscribedInstruments.length) {
      fse.outputJSONSync(
        KF_SUBSCRIBED_INSTRUMENTS_JSON_PATH,
        newSubscribedInstruments,
      );
    }
  }

  if (tag === 'req_instruments') {
    const instruments: InstrumentResolvedData = safeReadJsonSync(
      KF_INSTRUMENTS_PATH,
      {},
    );
    self.postMessage({
      updateTime: new Date().getTime(),
      instruments,
    });
  }
});
