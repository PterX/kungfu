const fse = require('fs-extra');
const path = require('path');
const {
  customResolve,
  getSdkDefaultDistDir,
} = require('@kungfu-trader/kungfu-js-api/toolkit/utils');

const templateInitPath = path.join(
  getSdkDefaultDistDir(),
  'sdk',
  'templates',
  'init',
);
const exampleTemplates = {
  strategy: [
    '@kungfu-trader/examples-strategy-python',
    '@kungfu-trader/examples-strategy-cpp',
  ],
  broker: [
    '@kungfu-trader/kfx-broker-sim',
    '@kungfu-trader/kfx-broker-xtp-demo',
  ],
};

const copyTemplate = (templateType, templateName) => {
  const templateDir = path.dirname(customResolve(templateName));
  const targetDir = path.join(
    templateInitPath,
    templateType,
    templateName.replace('@kungfu-trader/', ''),
  );

  try {
    fse.copySync(templateDir, targetDir);
    fse.removeSync(path.join(targetDir, 'node_modules'));
    fse.removeSync(path.join(targetDir, 'dist'));
  } catch (error) {
    console.error(`Error copying template ${templateName}: ${error}`);
  }
};

const copyAllTemplates = () => {
  Object.entries(exampleTemplates).forEach(([type, templates]) => {
    templates.forEach((templateName) => {
      copyTemplate(type, templateName);
    });
  });
};

copyAllTemplates();
