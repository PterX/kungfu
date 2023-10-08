export const SpecialWordsReg = new RegExp(
  "[`~!#$^&*()=|{}';'\\[\\]<>\"\\\\《》?~！#￥¥……&（）——|{}【】「」‘；｜：”“'。，、？_]+|(-+$)|(^-+)" +
    '|(?:[cC][oO][nN]|[pP][rR][nN]|[aA][uU][xX]|[nN][uU][lL]|[cC][oO][mM][0-9]|[lL][pP][tT][0-9])\\b' + // windows保留名称
    '|([\\u4E00-\\u9FFF]+)', // 中文
  'i',
);

export const EnterableSpecialWordsReg = new RegExp('[,.:/]+', 'ig');

export const InvalidFileNameReg =
  /^(?:[^a-zA-Z0-9\u4E00-\u9FA5_/:？*' '<>$|]|.*[^a-zA-Z0-9\u4E00-\u9FA5_/:？*' '<>$|]$|.*[/:？?*' <>，：｜|$\\].*|\b(?:[cC][oO][nN]|[pP][rR][nN]|[aA][uU][xX]|[nN][uU][lL]|[cC][oO][mM][0-9]|[lL][pP][tT][0-9])\b)/;
