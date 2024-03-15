import fse from 'fs-extra';
import { NG_CACHE_PATH } from '../configs';
import { NoviceCacheType } from '../typings';

export const setNoviceCache = (noviceCache: NoviceCacheType) => {
  return fse.writeJsonSync(NG_CACHE_PATH, noviceCache || {});
};

export const getNoviceCache = () => {
  return (fse.readJsonSync(NG_CACHE_PATH) || {}) as NoviceCacheType;
};

export const setNoviceCacheByKey = (key: string, value: boolean) => {
  const noviceCache = getNoviceCache();
  noviceCache[key] = value;
  return setNoviceCache(noviceCache);
};
