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
  const quotes: KungfuApi.Quote[] = [];
  const orders: KungfuApi.Order[] = [];
  const trades: KungfuApi.Trade[] = [];
  data.forEach((item) => {
    const resolvedData = jsonParse(JSON.stringify(item.data));
    switch (item.msgType) {
      case FrameMsgTypeEnum.Quote:
        quotes.push(resolvedData);
        break;
      case FrameMsgTypeEnum.Order:
        orders.push(resolvedData);
        break;
      case FrameMsgTypeEnum.Trade:
        trades.push(resolvedData);
        break;
    }
  });
  console.log('send-events', { trades, quotes, orders }, info);
  dataSender.sendData('send-trades', trades, info);
  dataSender.sendData('send-quotes', quotes, info);
  dataSender.sendData('send-orders', orders, info);
});

dataReceiver.onEnd<KungfuApi.FrameResolved>(
  'send-md-events',
  ({ data, info }) => {
    const quotes: KungfuApi.Quote[] = [];
    data.forEach((item) => {
      const resolvedData = jsonParse(JSON.stringify(item.data));
      switch (item.msgType) {
        case FrameMsgTypeEnum.Quote:
          quotes.push(resolvedData);
          break;
      }
    });
    dataSender.sendData('send-md-quotes', quotes, info);
  },
);
