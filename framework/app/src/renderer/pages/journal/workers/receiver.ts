/* eslint-disable @typescript-eslint/no-explicit-any */
export class WorkerReceiver {
  _typePrefix: string;
  _worker?: Worker;
  _data: Record<string, unknown[]> = {};
  callbacks: Record<
    string,
    {
      onMessage?: Array<
        (event: {
          data: any[];
          isEnd: boolean;
          info?: Record<string, any>;
        }) => void
      >;
      onEnd?: Array<
        (event: { data: any[]; info?: Record<string, any> }) => void
      >;
    }
  >;

  constructor(typePrefix: string, worker?: Worker) {
    this._typePrefix = typePrefix;
    this._worker = worker;
    this.callbacks = {};

    if (worker) {
      worker.onmessage = (event) => {
        this._onMessage(event.data || {});
      };
    } else {
      self.addEventListener('message', (event) => {
        this._onMessage(event.data || {});
      });
    }
  }

  _onMessage<T>(message: {
    type: string;
    data: { data: string; isEnd: boolean; info?: Record<string, any> };
  }) {
    const { data, type } = message;
    if (!type?.startsWith(this._typePrefix)) return;

    const resolvedData = JSON.parse(data.data) as T[];
    this._receiveData<T>(type, resolvedData || []);

    this.callbacks[type].onMessage?.forEach((cb) =>
      cb({ data: resolvedData, isEnd: data.isEnd, info: data.info }),
    );

    if (data.isEnd) {
      this.callbacks[type].onEnd?.forEach((cb) =>
        cb({ data: this._data[type], info: data.info }),
      );

      this._data[type] = [];
    }
  }

  _receiveData<T>(type: string, data: T[]) {
    if (!this._data[type]) this._data[type] = [];
    this._data[type].push(...data);
  }

  onMessage<T>(
    type: string,
    cb: (event: {
      data: T[];
      isEnd: boolean;
      info?: Record<string, any>;
    }) => void,
  ) {
    if (!this.callbacks[type]) this.callbacks[type] = { onMessage: [] };
    this.callbacks[type].onMessage?.push(cb);
  }

  onEnd<T>(
    type: string,
    cb: (event: { data: T[]; info?: Record<string, any> }) => void,
  ) {
    if (!this.callbacks[type]) this.callbacks[type] = { onEnd: [] };
    this.callbacks[type].onEnd?.push(cb);
  }
}
