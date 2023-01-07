import { WorkerReceiver } from './receiver';
import { WorkerSender } from './sender';
import { FrameMsgTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';

const dataReceiver = new WorkerReceiver('send');
const dataSender = new WorkerSender(self, 200);

const jsonParse = (json: string) => {
  try {
    const jsonResolved = JSON.parse(json);
    return jsonResolved;
  } catch (err) {
    if (err instanceof Error) {
      console.log(err.message);
    }
    return null;
  }
};

dataReceiver.onEnd<KungfuApi.FrameResolved>('send-events', ({ data, info }) => {
  const trades = data
    .filter((item) => item.msgType === FrameMsgTypeEnum.Trade)
    .map((trade) => jsonParse(trade.data)) as KungfuApi.Trade[];

  const quotes = data
    .filter((item) => item.msgType === FrameMsgTypeEnum.Quote)
    .map((quote) => jsonParse(quote.data)) as KungfuApi.Quote[];

  const orders = data
    .filter((item) => item.msgType === FrameMsgTypeEnum.Order)
    .map((order) => jsonParse(order.data)) as KungfuApi.Order[];

  dataSender.sendData('send-trades', trades, info);
  dataSender.sendData('send-quotes', quotes, info);
  dataSender.sendData('send-orders', orders, info);
});
