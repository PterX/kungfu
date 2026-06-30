const fse = require('fs-extra');
const path = require('path');
const { getCoreDir } = require('@kungfu-tech/api/toolkit/utils');
const corePath = getCoreDir();
const enumPath = path.join(
  corePath,
  'src',
  'include',
  'kungfu',
  'longfist',
  'enums.h',
);

const text = fse.readFileSync(enumPath).toString();

const enumRegex = /enum[\s\S]*?\};/g;
const classnamePattern = /enum\s+class\s+(\w+)\s*:\s*\w+\s*\{?/;
const enumContentRegex = /\{(\s*[\s\S]*?)\}/;

exports.parseEnums = () => {
  const matches = text.match(enumRegex);
  return matches
    .map((item) => {
      item = item.split('enums {').join(' ');

      const m = classnamePattern.exec(item);
      let title = '';
      if (m) {
        title = m[1];
      } else {
        console.warn(`cannot match title ${item}`);
      }

      const bodyMatches = enumContentRegex.exec(item);
      let bodyKeys = [];
      if (bodyMatches) {
        const bodyLines = bodyMatches[1].split('\n');
        const noCommentBodylines = bodyLines
          .map((item) => item.split('//').slice(0, 1).join(' '))
          .map((item) => item.split('=').slice(0, 1).join(' '))
          .map((item) => item.trim())
          .filter((item) => !!item)
          .map((item) => item.split(',').filter((item) => !!item));
        bodyKeys = noCommentBodylines.reduce((pre, item2) => {
          item2.forEach((key) => pre.push(key));
          return pre;
        }, []);
      } else {
        console.warn(`cannot match body ${item}`);
      }

      if (title && bodyKeys.length) {
        return {
          name: title,
          keys: bodyKeys,
        };
      } else {
        return null;
      }
    })
    .filter((item) => !!item);
};
