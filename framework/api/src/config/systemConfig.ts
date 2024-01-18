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

export const defaultColorMap = {
  red: '#F54747',
  green: '#15DE9E',
  orange: '#D87A16',
  blue: '#1890FF',
  yellow: '#FFFF00',
  purple: '#531DAB',
  gray: '#8C8C8C',
  pink: '#FFC0CB',
  cyan: '#00ffff',
  text: '#FFFFFFD9',
  default: '#FFFFFFD9',
  '#FAAD14': '#FAAD14',
  'kf-color-running': '#FAAD14',
  'kf-color-waiting': '#D87A16',
  'kf-color-error': '#F54747',
};
