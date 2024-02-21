class DynamicIndexedMap<K extends string | number, V> {
  private keyValueMap: { [key in K]?: V };
  private commonKeyIndexMap: { [key in K]?: number };
  private fullKeyIndexMap: { [key in K]?: number };
  private commonList: V[];
  private fullList: V[];
  private commonListOffset = 1;
  private commonSliceCount = 0;
  private fullListOffset = 1;
  // private listLength = 0;
  private finishCount = 0;
  private unFinishedCount = 0;
  private updatedUnfinishedIndexList: number[] = [];
  // private updateUnfinishedIndexCount = 0;
  private updateFinishedIndexList: number[] = [];
  private maxCommonListLength: number = 50000;
  private;

  constructor(maxLength = 500) {
    this.keyValueMap = {};
    this.commonKeyIndexMap = {};
    this.fullKeyIndexMap = {};
    this.commonList = [];
    this.fullList = [];
    this.maxCommonListLength = maxLength;
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
    const length = this.updateFinishedIndexList.length;
    if (length === 0) {
      return 0;
    }

    let left = 0;
    let right = length;
    while (left < right) {
      const mid = Math.floor((left + right) / 2);
      if (this.updateFinishedIndexList[mid] < num) {
        left = mid + 1;
      } else {
        right = mid;
      }
    }

    return left;
  }

  getOrderStatus(key: K) {
    const index = this.commonKeyIndexMap[key];
    if (index === undefined) {
      console.error(`Key ${key} not found in map`);
      return;
    }
    return index;
  }

  insertKeyWithValue(key: K, value: V, isFinished: boolean): void {
    if (this.maxCommonListLength <= this.commonList.length) {
      this.deleteLastCommonValue();
    }

    this.commonKeyIndexMap[key] = --this.commonListOffset; // 为新键分配当前偏移量作为索引 插入新元素后减少偏移量
    this.commonList.unshift(value);
    this.keyValueMap[key] = value;
    if (!isFinished) {
      this.fullKeyIndexMap[key] = --this.fullListOffset;
      this.fullList.unshift(value);
    }
  }
  updateKeyWithValue(key: K, value: V, isFinished: boolean): void {
    const correctIndex = this.getCommonListIndexForKey(key);
    if (correctIndex !== undefined) {
      this.keyValueMap[key] = value;
      this.commonList.splice(correctIndex, 1, value);
    } else {
      return;
    }

    const fullCorrectIndex = this.getFullListIndexForKey(key);
    if (fullCorrectIndex !== undefined) {
      if (isFinished) {
        this.fullList.splice(fullCorrectIndex, 1);
        this.updateFinishedIndexList.push(this.fullKeyIndexMap[key] || 0);
        delete this.fullKeyIndexMap[key];
      } else {
        this.fullList.splice(fullCorrectIndex, 1, value);
      }
    }
  }

  deleteLastCommonValue(): void {
    this.commonList.pop();
    this.commonSliceCount++;
  }

  getCommonListIndexForKey(key: K): number | undefined {
    const index = this.commonKeyIndexMap[key];

    if (index === undefined) {
      return undefined;
    }
    if (Number(index) + this.commonSliceCount > 0) {
      delete this.commonKeyIndexMap[key];
      return undefined;
    }
    return Number(index) - this.commonListOffset;
  }

  getFullListIndexForKey(key: K): number | undefined {
    const index = this.fullKeyIndexMap[key];
    if (index === undefined) {
      return;
    }
    return (
      Number(index) - this.fullListOffset - this.countSmallerNumbers(index)
    );
  }

  hasKey(key: K): boolean {
    return (
      this.commonKeyIndexMap[key] !== undefined ||
      this.fullKeyIndexMap[key] !== undefined
    );
  }

  getValueForKey(key: K): V | undefined {
    return this.keyValueMap[key];
  }

  getIndexForKey(key: K): number | undefined {
    const index = this.commonKeyIndexMap[key];
    if (index === undefined) {
      console.error(`Key ${key} not found in map`);
      return;
    }
    if (Number(index) <= 0) {
      const countUpdated = this.countSmallerNumbers(index);
      return Number(index) - this.commonListOffset - countUpdated;
    } else {
      return Number(index);
    }
  }
  getKeyIndexMap(): { [key in K]?: number } {
    return this.commonKeyIndexMap;
  }

  getValuesArrayLength(): number {
    return this.commonList.length;
  }

  getUnfinishEdListLength(): number {
    return this.unFinishedCount;
  }

  getFinishedListLength(): number {
    return this.finishCount;
  }
  getCommonList(): V[] {
    return [...this.commonList];
  }

  // popLastFinishedValue(): void | undefined {
  //   if (this.finishCount > 0) {
  //     this.finishCount--;
  //     this.listLength--;
  //     this.commonList.pop();
  //   }
  // }

  getFullList(): V[] {
    return [...this.fullList];
  }
}

export { DynamicIndexedMap };
