export type NoviceCacheType = Record<string, boolean>;

export interface AutoAddConfig {
  location: KungfuApi.KfLocation;
  initValue?: Record<string, KungfuApi.KfConfigValue>;
}

export type AutoAddConfigs = AutoAddConfig[];
