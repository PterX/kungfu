const fse = require('fs-extra');
const path = require('path');
const ignore = require('ignore');
const {
  customResolve,
  getSdkDefaultDistDir,
} = require('@kungfu-tech/api/toolkit/utils');

const templateInitPath = path.join(
  getSdkDefaultDistDir(),
  'sdk',
  'templates',
  'init',
);

const exampleTemplates = {
  strategy: [
    '@kungfu-tech/examples-strategy-python',
    '@kungfu-tech/examples-strategy-cpp',
  ],
  broker: [
    '@kungfu-tech/kfx-broker-sim',
    '@kungfu-tech/kfx-broker-xtp-demo',
  ],
};

const rootDir = path.resolve(__dirname, '../../../../');
const rootGitignorePath = path.join(rootDir, '.gitignore');
let rootGitignoreContent = '';

try {
  rootGitignoreContent = fse.readFileSync(rootGitignorePath, 'utf8');
} catch (error) {
  console.error(`Error reading root .gitignore: ${error}`);
}

const removeEmptyDirectories = (directory, ig) => {
  try {
    if (!fse.statSync(directory).isDirectory()) {
      return;
    }

    let files = fse.readdirSync(directory);
    files.forEach((file) => {
      removeEmptyDirectories(path.join(directory, file), ig);
    });

    if (
      fse.readdirSync(directory).length === 0 &&
      ig.ignores(path.relative(rootDir, directory))
    ) {
      fse.rmdirSync(directory);
    }
  } catch (error) {
    console.error(`Error removing empty directory: ${error}`);
  }
};

const copyTemplate = (templateType, templateName) => {
  try {
    const templateDir = path.dirname(customResolve(templateName));
    const targetDir = path.join(
      templateInitPath,
      templateType,
      templateName.replace('@kungfu-tech/', ''),
    );

    const gitignorePath = path.join(templateDir, '.gitignore');
    let gitignoreContent = '';

    if (fse.existsSync(gitignorePath)) {
      gitignoreContent = fse.readFileSync(gitignorePath, 'utf8');
    }

    const ig = ignore().add(rootGitignoreContent + '\n' + gitignoreContent);

    fse.copySync(templateDir, targetDir, {
      filter: (src) => !ig.ignores(path.relative(rootDir, src)),
    });
    removeEmptyDirectories(targetDir, ig);
  } catch (error) {
    console.error(`Error copying template ${templateName}: ${error}`);
  }
};

const copyAllTemplates = () => {
  Object.entries(exampleTemplates).forEach(([type, templates]) => {
    templates.forEach((templateName) => copyTemplate(type, templateName));
  });
};

try {
  copyAllTemplates();
} catch (error) {
  console.error(`Error during template copying: ${error}`);
}
