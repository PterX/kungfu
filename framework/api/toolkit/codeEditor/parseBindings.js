const fse = require('fs-extra');
const path = require('path');
const { getCoreDir } = require('@kungfu-tech/api/toolkit/utils');
const corePath = getCoreDir();
const bindingsDir = path.join(corePath, 'src', 'bindings', 'python', 'binding');

const propertyPattern = /def_property_readonly\("([^"]*)",/;
const readonlyPattern = /def_readonly\("([^"]*)",/;
const submodulePattern = /def_submodule\("([^"]*)"\)/;
const functionPattern = /def\("([^"]*)",/;

exports.parseBindings = () => {
  const propertyMatchesDict = {};
  const readonlyMatchesDict = {};
  const submoduleMatchesDict = {};
  const functionMatchesDict = {};

  fse
    .readdirSync(bindingsDir)
    .filter((file) => file.endsWith('.cpp'))
    .forEach((file) => {
      const contents = fse
        .readFileSync(path.join(bindingsDir, file))
        .toString()
        .split('\n');

      contents.forEach((line) => {
        const propertyMatches = line.match(propertyPattern);
        if (propertyMatches) propertyMatchesDict[propertyMatches[1]] = true;
        const readonlyMatches = line.match(readonlyPattern);
        if (readonlyMatches) readonlyMatchesDict[readonlyMatches[1]] = true;
        const submoduleMatches = line.match(submodulePattern);
        if (submoduleMatches) submoduleMatchesDict[submoduleMatches[1]] = true;
        const functionMatches = line.match(functionPattern);
        if (functionMatches) functionMatchesDict[functionMatches[1]] = true;
      });
    });

  return {
    property: Object.keys({
      ...propertyMatchesDict,
      ...readonlyMatchesDict,
      ...submoduleMatchesDict,
    }),
    functions: Object.keys(functionMatchesDict),
  };
};
