import { KfCategoryEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';

export const SpecialWordsReg = new RegExp(
  "[`~!#$^&*()=|{}';'\\[\\]\\t<>\"\\\\《》?~！#￥¥……&（）——|{}【】「」‘；｜：”“'。，、？_ ]+|(-+$)|(^-+)" +
    '|(?:[cC][oO][nN]|[pP][rR][nN]|[aA][uU][xX]|[nN][uU][lL]|[cC][oO][mM][0-9]|[lL][pP][tT][0-9])\\b' + // windows保留名称
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

export const InvalidFileNameReg =
  /^(?:[^a-zA-Z0-9\u4E00-\u9FA5_/:？*' '<>$|]|.*[^a-zA-Z0-9\u4E00-\u9FA5_/:？*' '<>$|]$|.*[/:？?*' <>，：｜|$\\].*|\b(?:[cC][oO][nN]|[pP][rR][nN]|[aA][uU][xX]|[nN][uU][lL]|[cC][oO][mM][0-9]|[lL][pP][tT][0-9])\b)/;

export const keyShortMap = {
  MakeOrder: 'CommandOrControl+Shift+1',
  FutureArbitrage: 'CommandOrControl+Shift+2',
  BlockTrade: 'CommandOrControl+Shift+9',
};
