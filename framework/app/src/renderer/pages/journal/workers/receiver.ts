export class WorkerReceiver<T> {
  _data: T[] = [];
  callbacks: Record<
    string,
    {
      onMessage?: (event: { data: T[]; isEnd: boolean }) => void;
      onEnd?: (data: T[]) => void;
    }
  >;

  constructor() {
    this.callbacks = {};
    self.addEventListener('message', (event) => {
      this._onMessage(event.data || {});
    });
  }

  _onMessage(message: {
    type: string;
    data: { data: string; isEnd: boolean };
  }) {
    console.log(message);
    const { data, type } = message;
    if (!type.startsWith('send')) return;
    const resolvedData = JSON.parse(data.data) as T[];
    this._receiveData(resolvedData || []);
    this.callbacks[type].onMessage?.({ data: resolvedData, isEnd: data.isEnd });
    if (data.isEnd) this.callbacks[type].onEnd?.(this._data);
  }

  _receiveData(data: T[]) {
    this._data.push(...data);
  }

  onMessage(type: string, cb: (event: { data: T[]; isEnd: boolean }) => void) {
    if (!this.callbacks[type]) this.callbacks[type] = {};
    this.callbacks[type].onMessage = cb;
  }

  onEnd(type, cb: (data: T[]) => void) {
    if (!this.callbacks[type]) this.callbacks[type] = {};
    this.callbacks[type].onEnd = cb;
  }
}
