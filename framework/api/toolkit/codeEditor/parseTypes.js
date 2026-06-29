const fse = require('fs-extra');
const path = require('path');
const { getCoreDir } = require('@kungfu-tech/api/toolkit/utils');
const corePath = getCoreDir();
const typesPath = path.join(
  corePath,
  'src',
  'include',
  'kungfu',
  'longfist',
  'types.h',
);

const text = fse.readFileSync(typesPath).toString();

exports.parseTypeAttrs = () => {
  let lines = text.split('\n');
  lines = lines
    .filter((item) => {
      return item.includes(' (') && item.includes('), ');
    })
    .map((item) => {
      const resolvedItem = item.split('),').slice(0, 1).join('').split(',');
      return resolvedItem
        .slice(resolvedItem.length - 1, resolvedItem.length)
        .join('')
        .trim();
    })
    .reduce((pre, key) => {
      pre[key] = true;
      return pre;
    }, {});
  return Object.keys(lines);
};

exports.parseTypename = () => {
  const typenames = [];
  const lines = text
    .split('\n')
    .join('')
    .split('//')
    .join('')
    .split(' ')
    .join('');
  const packTypePattern = /KF_DEFINE_PACK_TYPE\(([^,]+),/g;
  const dataTypePattern = /KF_DEFINE_DATA_TYPE\(([^,]+),/g;

  const packTypeMatches = lines.matchAll(packTypePattern);
  for (const match of packTypeMatches) {
    typenames.push(match[1]);
  }

  const dataTypeMatches = lines.matchAll(dataTypePattern);
  for (const match of dataTypeMatches) {
    typenames.push(match[1]);
  }

  return typenames.filter((item) => !!item);
};
