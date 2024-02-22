const path = require('path');
const fse = require('fs-extra');
const { parseTypeAttrs, parseTypename } = require('./parseTypes');
const { parseEnums } = require('./parseEnums');
const { parseBindings } = require('./parseBindings');
const { getAppDir } = require('../utils');

const enums = parseEnums();
const bindings = parseBindings();
const typeAttrs = parseTypeAttrs();
const typenames = parseTypename();

const keyWordsDir = path.join(getAppDir(), 'public', 'keywords');

const functions = bindings.functions;
const flatternEnums = enums
  .map((item) => {
    const { name, keys } = item;
    return [...keys, name];
  })
  .reduce((pre, list) => {
    pre = [...pre, ...list];
    return pre;
  }, []);

const properties = Object.keys(
  [...bindings.property, ...typeAttrs, ...typenames, ...flatternEnums].reduce(
    (pre, key) => {
      pre[key] = true;
      return pre;
    },
    {},
  ),
);

const keywords = Object.keys(
  [
    ...bindings.property,
    ...typeAttrs,
    ...typenames,
    ...functions,
    ...flatternEnums,
  ].reduce((pre, key) => {
    pre[key] = true;
    return pre;
  }, {}),
);

const kungfuKeywords = path.join(keyWordsDir, 'kungfuKeywords');
const kungfuFunctions = path.join(keyWordsDir, 'kungfuFunctions');
const kungfuProperties = path.join(keyWordsDir, 'kungfuProperties');

exports.generateCodeKeyWords = () => {
  fse.writeFileSync(
    kungfuKeywords,
    keywords.map((item) => item.trim()).join('\n'),
  );
  fse.writeFileSync(
    kungfuFunctions,
    functions.map((item) => item.trim()).join('\n'),
  );
  fse.writeFileSync(
    kungfuProperties,
    properties.map((item) => item.trim()).join('\n'),
  );
};
