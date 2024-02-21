class DynamicIndexedMap<K extends string | number, V> {
  private keyValueMap: { [key in K]?: V };
  private keyIndexMap: { [key in K]?: number };
  private values: V[];
  private offset = 1;
  private listLength = 0;
  private finishCount = 0;
  private unFinishedCount = 0;
  private updatedUnfinishedIndexList: number[] = [];
  private updateUnfinishedIndexCount = 0;

  constructor() {
    this.keyValueMap = {};
    this.keyIndexMap = {};
    this.values = [];
  }

  insertNegativeNumber(num) {
    if (this.updatedUnfinishedIndexList.length === 0) {
      return this.updatedUnfinishedIndexList.push(num);
    }

    let left = 0;
    let right = this.updatedUnfinishedIndexList.length - 1;
    while (left <= right) {
      const mid = Math.floor((left + right) / 2);
      if (this.updatedUnfinishedIndexList[mid] < num) {
        left = mid + 1;
      } else {
        right = mid - 1;
      }
    }

    this.updatedUnfinishedIndexList.splice(left, 0, num);

    return this.updatedUnfinishedIndexList;
  }

  countSmallerNumbers(num) {
    if (this.updatedUnfinishedIndexList.length === 0) {
      return 0;
    }

    let left = 0;
    let right = this.updatedUnfinishedIndexList.length;
    while (left < right) {
      const mid = Math.floor((left + right) / 2);
      if (this.updatedUnfinishedIndexList[mid] < num) {
        left = mid + 1;
      } else {
        right = mid;
      }
    }

    return left;
  }

  getOrderStatus(key: K) {
    const index = this.keyIndexMap[key];
    if (index === undefined) {
      console.error(`Key ${key} not found in map`);
      return;
    }
    return index;
  }

  insertKeyWithValue(key: K, value: V, atStart = true): void {
    if (atStart) {
      this.keyIndexMap[key] = --this.offset; // 为新键分配当前偏移量作为索引 插入新元素后减少偏移量
      this.values.unshift(value); // 在数组前端插入值
      this.listLength++;
      this.keyValueMap[key] = value;
      this.unFinishedCount++;
    } else {
      this.finishCount = this.listLength + this.offset;
      this.values.splice(
        1 - this.offset - this.updateUnfinishedIndexCount,
        0,
        value,
      );

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
    const correctIndex =
      Number(index) - this.offset - this.countSmallerNumbers(index);
    this.values.splice(correctIndex, 1, value);
  }

  deleteUnfinishedKeyAndValue(key: K): void {
    const realIndex = this.getIndexForKey(key);
    if (realIndex === undefined) {
      console.error(`Key ${key} not found in map`);
      return;
    }
    this.values.splice(realIndex, 1);
    const index = this.keyIndexMap[key];

    this.insertNegativeNumber(index);
    this.updateUnfinishedIndexCount++;
    console.log('index', index);
    delete this.keyIndexMap[key];
  }
  deleteFinishedKeyAndValue(key: K) {
    const index = this.keyIndexMap[key];
    if (index === undefined) {
      console.error(`Key ${key} not found in map`);
      return;
    }
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
    if (Number(index) <= 0) {
      const countUpdated = this.countSmallerNumbers(index);
      return Number(index) - this.offset - countUpdated;
    } else {
      return Number(index);
    }
  }
  getKeyIndexMap(): { [key in K]?: number } {
    return this.keyIndexMap;
  }

  getValuesArray(): V[] {
    return [...this.values];
  }

  getValuesArrayLength(): number {
    return this.values.length;
  }

  getUnfinishEdListLength(): number {
    return this.unFinishedCount;
  }

  getFinishedListLength(): number {
    return this.finishCount;
  }

  popLastFinishedValue(): void | undefined {
    if (this.finishCount > 0) {
      this.finishCount--;
      this.listLength--;
      this.values.pop();
    }
  }
}

export { DynamicIndexedMap };
