export class WorkerSender<T> {
  worker: Worker;
  limit: number;

  constructor(worker: Worker, limit: number) {
    this.worker = worker;
    this.limit = limit;
  }

  sendData(data: T[]) {
    const len = data.length;
    const times = Math.ceil(len / this.limit);

    for (let i = 1, s = 0; i <= times; i++, s += this.limit) {
      const e = s + Math.min(this.limit, len - s);
      const curData = data.slice(s, e);
      const isEnd = e === len;

      const sendData = this.createSendData(curData, isEnd);

      this.worker.postMessage(sendData);
    }
  }

  createSendData(data: T[], isEnd: boolean) {
    return {
      isEnd,
      data,
    };
  }
}
