import { KfCategoryEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';

export const SpecialWordsReg = new RegExp(
  "[`~!#$^&*()=|{}';'\\[\\]<>\"\\\\《》?~！#￥¥……&（）——|{}【】「」‘；｜：”“'。，、？_]+|(-+$)|(^-+)" +
    '|^CON$|^PRN$|^AUX$|^NUL$|^COM1$|^COM2$|^COM3$|^COM4$|^COM5$|^COM6$|^COM7$|^COM8$|^COM9$|^LPT1$|^LPT2$|^LPT3$|^LPT4$|^LPT5$|^LPT6$|^LPT7$|^LPT8$|^LPT9$' + // windows保留名称
    '|([\\u4E00-\\u9FFF]+)', // 中文
  'i',
);

export const EnterableSpecialWordsReg = new RegExp('[,.:/]+', 'ig');

export const KfCategoryNameMap = {
  [KfCategoryEnum.md]: 'md',
  [KfCategoryEnum.td]: 'td',
  [KfCategoryEnum.strategy]: 'strategy',
  [KfCategoryEnum.system]: 'system',
  [KfCategoryEnum.operator]: 'operator',
};

export const invalidFileNameReg =
  /^(con|prn|aux|nul|com[0-9]$|lpt[0-9]$)$|[\0-\x1F\x80-\x9F\\/:*?"<>|]/i;
