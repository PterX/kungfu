export const SpecialWordsReg = new RegExp(
  "[`~!#$^&*()=|{}';'\\[\\]<>\"\\\\《》?~！#￥¥……&（）——|{}【】「」‘；｜：”“'。，、？_]+|(-+$)|(^-+)" +
    '|^CON$|^PRN$|^AUX$|^NUL$|^COM1$|^COM2$|^COM3$|^COM4$|^COM5$|^COM6$|^COM7$|^COM8$|^COM9$|^LPT1$|^LPT2$|^LPT3$|^LPT4$|^LPT5$|^LPT6$|^LPT7$|^LPT8$|^LPT9$' + // windows保留名称
    '|([\\u4E00-\\u9FFF]+)', // 中文
  'i',
);

export const EnterableSpecialWordsReg = new RegExp('[,.:/]+', 'ig');

export const EmojiRegex = /\p{Emoji}/u;

export const InvalidFileNameReg =
  /^(?:[^a-zA-Z0-9_/:？*' '<>$|]|.*[^a-zA-Z0-9_/:？*' '<>$|]$|.*[/:？?*' <>，：｜|$\\].*|\b(?:[cC][oO][nN]|[pP][rR][nN]|[aA][uU][xX]|[nN][uU][lL]|[cC][oO][mM][0-9]|[lL][pP][tT][0-9])\b)/;
