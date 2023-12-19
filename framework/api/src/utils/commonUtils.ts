// 此文件内只放 不依赖外部逻辑 的 纯函数

export const booleanProcessEnv = (
  val: string | boolean | undefined,
): boolean => {
  if (val === undefined) {
    return false;
  }

  if (val === 'null') {
    return false;
  }

  if (val === 'true') {
    return true;
  } else if (val === 'false') {
    return false;
  } else {
    return !!val;
  }
};

export const ifKfDev = () => booleanProcessEnv(process.env.IS_KF_DEV);

export const getNaturalNumber = <T extends number | bigint>(num: T): T => {
  if (typeof num === 'bigint') {
    return num > 0n ? num : (0n as T);
  }

  return num > 0 ? num : (0 as T);
};

export const omitObject = <T>(obj: T, keys: Array<keyof T>) => {
  const strKeys = keys.map((key) => key.toString());
  return Object.keys(obj)
    .filter((key) => !strKeys.includes(key))
    .reduce((result, key) => {
      result[key] = obj[key];
      return result;
    }, {});
};
