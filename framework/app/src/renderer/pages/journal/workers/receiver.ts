export class Receiver<T> {
  worker: Worker;

  data: T[] = [];
  onEndCallback?: (data: T[]) => void;

  constructor(worker: Worker) {
    this.worker = worker;

    self.addEventListener('message', (event) => {
      this._onMessage(event.data || {});
    });
  }

  _onMessage({ data, isEnd }: { data: T[]; isEnd: false }) {
    this._receiveData(data || []);
    if (isEnd && this.onEndCallback) this.onEndCallback(this.data);
  }

  _receiveData(data: T[]) {
    this.data.push(...data);
  }

  onEnd(cb: (data: T[]) => void) {
    this.onEndCallback = cb;
  }
}
