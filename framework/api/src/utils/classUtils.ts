class DynamicIndexedMap<K extends string | number, V> {
  private keyValueMap: { [key in K]?: V };
  private keyIndexMap: { [key in K]?: number };
  private values: V[];
  private offset: number = 1;
  private listLength: number = 0;
  private finishCount: number = 0;
  private unFinishedCount: number = 0;

  constructor() {
    this.keyValueMap = {};
    this.keyIndexMap = {};
    this.values = [];
  }

  insertKeyWithValue(
    key: K,
    value: V,
    atStart: boolean = true,
    length = 500,
  ): void {
    if (atStart) {
      this.keyIndexMap[key] = --this.offset; // 为新键分配当前偏移量作为索引 插入新元素后减少偏移量
      this.values.push(value); // 在数组前端插入值
      this.listLength++;
      this.keyValueMap[key] = value;
      this.unFinishedCount++;
    } else {
      this.finishCount = this.listLength + this.offset;
      if (this.finishCount >= length) return;
      this.values.splice(0 - this.offset + 1, 0, value);

      this.keyIndexMap[key] = this.finishCount;
      this.listLength++;
      this.keyValueMap[key] = value;
    }
  }
  updateKeyWithValue(key: K, value: V): void {
    const index = this.keyIndexMap[key];
    if (index === undefined) {
      console.error(`Key ${key} not found in map`);
      return;
    }
    this.keyValueMap[key] = value;
    const correctIndex = Number(index) - this.offset;
    this.values.splice(correctIndex, 1, value);
  }
  hasKey(key: K): boolean {
    return this.keyValueMap[key] !== undefined;
  }

  getValueForKey(key: K): V | undefined {
    return this.keyValueMap[key];
  }

  getIndexForKey(key: K): number | undefined {
    const index = this.keyIndexMap[key];
    if (index === undefined) {
      console.error(`Key ${key} not found in map`);
      return;
    }
    return Number(index) <= 0
      ? Number(index) - this.offset
      : this.listLength - Number(index);
  }
  getKeyIndexMap(): { [key in K]?: number } {
    return this.keyIndexMap;
  }

  getValuesArray(): V[] {
    return [...this.values];
  }

  getUnfinishEdListLength(): number {
    return this.unFinishedCount;
  }

  getFinishedListLength(): number {
    return this.finishCount;
  }
}

export { DynamicIndexedMap };
