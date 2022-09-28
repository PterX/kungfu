export class WorkerSender<T> {
  worker: Worker;
  limit: number;

  constructor(worker: Worker, limit: number) {
    this.worker = worker;
    this.limit = limit;
  }

  sendData(type: string, data: T[]) {
    const len = data.length;
    const times = Math.ceil(len / this.limit);

    for (let i = 1, s = 0; i <= times; i++, s += this.limit) {
      const e = s + Math.min(this.limit, len - s);
      const curData = data.slice(s, e);
      const isEnd = e === len;

      const sendData = this._createSendData(type, curData, isEnd);

      this.worker.postMessage(sendData);
    }
  }

  _createSendData(type: string, data: T[], isEnd: boolean) {
    return {
      type: type,
      data: {
        isEnd,
        data: this._dealData(data),
      },
    };
  }

  _dealData(data: T[]) {
    return JSON.stringify(data);
  }
}
