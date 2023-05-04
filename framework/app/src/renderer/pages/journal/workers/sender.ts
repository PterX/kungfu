/* eslint-disable @typescript-eslint/no-explicit-any */
export class WorkerSender<T> {
  worker: Worker | Window;
  limit: number;

  constructor(worker: Worker | Window, limit: number) {
    this.worker = worker;
    this.limit = limit;
  }
  sendData(type: string, data: T[], info?: Record<string, any>) {
    const len = data.length;
    if (!len) {
      const sendData = this._createSendData(type, [], true, info);

      this.worker.postMessage(sendData);
      return;
    }
    const times = Math.ceil(len / this.limit);

    for (let i = 1, s = 0; i <= times; i++, s += this.limit) {
      const e = s + Math.min(this.limit, len - s);
      const curData = data.slice(s, e);
      const isEnd = e === len;

      const sendData = this._createSendData(type, curData, isEnd, info);

      this.worker.postMessage(sendData);
    }
  }

  _createSendData(
    type: string,
    data: T[],
    isEnd: boolean,
    info?: Record<string, any>,
  ) {
    return {
      type: type,
      data: {
        isEnd,
        data: this._dealData(data),
        info: info || {},
      },
    };
  }

  _dealData(data: T[]) {
    return JSON.stringify(data, function (_, value) {
      if (typeof value === 'bigint') {
        return value + '';
      }
      return value;
    });
  }
}
