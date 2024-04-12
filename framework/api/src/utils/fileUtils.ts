import os from 'os';
import path from 'path';
import fse, { Stats } from 'fs-extra';
import fsPromise from 'fs/promises';
import * as csv from 'fast-csv';
import { FormatterRow, ParserOptionsArgs } from 'fast-csv';
import findRoot from 'find-root';
import { RootConfigJSON } from '../typings/global';
import {
  buildRuntimeChildDirByType,
  RuntimeChildDirTypes,
} from '../config/pathConfig';
import { ifKfDev } from './commonUtils';

//添加文件
export const addFileSync = (
  parentDir = '',
  filename: string,
  type: 'folder' | 'file',
  content?: string,
): void => {
  let targetPath: string;
  if (!parentDir) targetPath = filename;
  else targetPath = path.join(parentDir, filename);
  targetPath = path.normalize(targetPath);

  if (isDiskRootDirectory(targetPath)) {
    return;
  }

  if (type === 'folder') {
    fse.ensureDirSync(targetPath);
  } else {
    fse.ensureFileSync(targetPath);
    if (content) {
      fse.writeFileSync(targetPath, content);
    }
  }
};

export const readCSV = <T>(
  filepath: string,
  headers: ParserOptionsArgs['headers'],
  options?: {
    transformer?: (row) => T;
    validator?: (row) => boolean;
  },
) => {
  filepath = path.normalize(filepath);
  return new Promise<{
    resRows: T[];
    errRows: Array<{ row: number; data: Array<string | number | boolean> }>;
  }>((resolve, reject) => {
    const resRows: T[] = [];
    const errRows: Array<{
      row: number;
      data: Array<string | number | boolean>;
    }> = [];

    let parsing = csv.parseFile(filepath, {
      headers: headers,
      skipLines: headers === true ? 0 : 1,
      trim: true,
    });

    if (options?.validator) {
      parsing = parsing.validate(options.validator);
    }

    parsing
      .on('data', function (row) {
        resRows.push(options?.transformer ? options.transformer(row) : row);
      })
      .on('data-invalid', function (data, row) {
        errRows.push({
          data,
          row,
        });
      })
      .on('end', function () {
        resolve({ resRows, errRows });
      })
      .on('error', (err) => {
        reject(err);
      });
  });
};

/**
 * 返回的 `stream` 通过 `'finished'` 事件判断写入`csv`结束：
 * ```
 * csvStream.on('finished', () => {
 *   ...
 * });
 * ```
 */
export const createWriteCsvStream = <T extends FormatterRow>(
  filePath: string,
  headers: boolean | string[],
  transform?: (row: T) => FormatterRow,
): csv.CsvFormatterStream<T, csv.FormatterRow> => {
  const csvStream = csv.format({ headers, transform });
  const fileWriteStream = fse.createWriteStream(path.normalize(filePath));
  // 解决Excel导出乱码的问题
  fileWriteStream.write(Buffer.from('\xEF\xBB\xBF', 'binary'));
  csvStream
    .on('data', (chunk) => {
      fileWriteStream.write(chunk);
    })
    .on('end', () => {
      fileWriteStream.end(() => {
        fileWriteStream.close((err) => {
          if (!err) {
            csvStream.emit('finished');
          } else {
            console.error(err);
          }
        });
      });
    });
  fileWriteStream.on('error', (err) => {
    csvStream.emit('error', err);
  });
  return csvStream;
};

export const writeCsvWithUTF8Bom = <T extends FormatterRow>(
  filePath: string,
  rows: T[],
  headers: boolean | string[],
  transform = (row: T) => row as FormatterRow,
) => {
  filePath = path.normalize(filePath);
  return new Promise<void>((resolve, reject) => {
    const csvStream = createWriteCsvStream(filePath, headers, transform);

    csvStream.on('finished', () => {
      resolve();
    });

    csvStream.on('error', (err) => {
      console.error(err);
      reject(err);
    });

    rows.forEach((row) => {
      csvStream.write(row);
    });
    csvStream.end();
  });
};

export const writeCSV = (
  filePath: string,
  data: KungfuApi.TradingDataTypes[],
  transform = (row: KungfuApi.TradingDataTypes) => row as FormatterRow,
): Promise<void> => {
  filePath = path.normalize(filePath);
  return new Promise((resolve, reject) => {
    csv
      .writeToPath(filePath, data, {
        headers: true,
        transform,
      })
      .on('finish', function () {
        resolve();
      })
      .on('error', (err) => {
        reject(err);
      });
  });
};

//获取文件内容
export const getFileContent = (targetPath: string): Promise<string> => {
  if (!targetPath || !fse.existsSync(targetPath))
    throw new Error(`${targetPath} not existed!`);
  targetPath = path.normalize(targetPath);
  return new Promise((resolve, reject): void => {
    const file = fse.createReadStream(targetPath);
    const fileContextList: Array<Buffer> = [];
    file.on('data', (data) => {
      fileContextList.push(data as Buffer);
    });
    file.on('end', () => {
      resolve(Buffer.concat(fileContextList).toString());
    });
    file.on('error', (err) => {
      reject(err);
      return;
    });
  });
};

export const listDir = (filePath: string): Promise<void | string[]> => {
  fse.ensureDirSync(filePath);
  return fse.readdir(filePath).catch((err) => console.error(err));
};

export const removeFileFolder = (targetPath: string): Promise<void> => {
  targetPath = path.normalize(targetPath);
  return new Promise((resolve) => {
    if (!fse.existsSync(targetPath)) {
      resolve();
      return;
    }
    fse.removeSync(targetPath);
    resolve();
  });
};

export const removeFilesInFolder = (targetDir: string) => {
  targetDir = path.normalize(targetDir);
  if (!fse.existsSync(targetDir)) throw new Error(`${targetDir} not existed!`);
  const promises = fse.readdirSync(targetDir).map((file: string) => {
    const filePath = path.join(targetDir, file);
    return removeFileFolder(filePath);
  });

  return Promise.all(promises);
};

export const listDirSync = (filePath: string): string[] => {
  fse.ensureDirSync(filePath);
  return fse.readdirSync(filePath);
};

export const removeTargetFoldersInFolder = async (
  targetFolder: string,
  includes: string[],
  filters: string[] = [],
): Promise<{ successes: string[]; errors: string[] }> => {
  const results: { successes: string[]; errors: string[] } = {
    successes: [],
    errors: [],
  };
  const iterator = async (folder: string) => {
    const items = listDirSync(folder);

    if (!items) return;

    const folders = items.filter((f: string) => {
      const stat = fse.statSync(path.join(folder, f));

      if (stat.isDirectory() && !filters.includes(f)) return true;
      return false;
    });

    for (const f of folders) {
      if (includes.includes(f)) {
        try {
          const targetFolder = path.join(folder, f);
          await fsPromise.rm(targetFolder, {
            force: true,
            recursive: true,
            maxRetries: 10,
          });
          results.successes.push(targetFolder);
        } catch (error) {
          if (error instanceof Error) {
            console.error(error);
            results.errors.push(error.message);
          }
        }
      } else {
        await iterator(path.join(folder, f));
      }
    }
  };

  await iterator(targetFolder);

  return results;
};

export const removeTargetFilesInFolder = async (
  targetFolder: string,
  includes: string[],
  filters: string[] = [],
): Promise<{ successes: string[]; errors: string[] }> => {
  const results: { successes: string[]; errors: string[] } = {
    successes: [],
    errors: [],
  };
  const iterator = async (folder: string) => {
    const items = listDirSync(folder);

    if (!items) return;

    const folders = items.filter((f: string) => {
      const stat = fse.statSync(path.join(folder, f));

      if (stat.isDirectory() && !filters.includes(f)) return true;
      return false;
    });

    const files = items.filter((f: string) => {
      const stat = fse.statSync(path.join(folder, f));

      if (stat.isFile()) return true;
      return false;
    });

    for (const f of files) {
      for (const n of includes) {
        if (f.includes(n) && !filters.some((filter) => f.includes(filter))) {
          try {
            const targetFile = path.join(folder, f);
            await fsPromise.rm(targetFile);
            results.successes.push(targetFile);
          } catch (error) {
            if (error instanceof Error) {
              console.error(error);
              results.errors.push(error.message);
            }
          }
        }
      }
    }

    for (const f of folders) {
      await iterator(path.join(folder, f));
    }
  };

  await iterator(targetFolder);

  return results;
};

export const findPackageRoot = () => {
  let searchPath = '';
  const cwd = process.cwd().toString();
  if (process.env.NODE_ENV === 'production') {
    searchPath = process.env.KF_APP_RUNTIME_DIR || __dirname;
  } else {
    searchPath = cwd;
  }
  if (searchPath.includes('node_modules')) {
    return findRoot(path.resolve(searchPath.split('node_modules')[0]));
  }
  return findRoot(path.resolve(searchPath));
};

export const readRootPackageJsonSync = (): RootConfigJSON => {
  if (globalThis.rootPackageJson) return globalThis.rootPackageJson;
  const rootDir = findPackageRoot();
  const packageJsonPath = path.join(rootDir, 'package.json');
  if (fse.existsSync(packageJsonPath)) {
    try {
      globalThis.rootPackageJson = fse.readJSONSync(packageJsonPath);
      return globalThis.rootPackageJson;
    } catch (err) {
      console.error(err);
      return {};
    }
  }

  return {};
};

export const isDiskRootDirectory = (dirPath: string): boolean => {
  const absolutePath = path.resolve(dirPath);
  const rootDirectory = path.parse(absolutePath).root;

  return absolutePath === rootDirectory;
};

export const getAppRuntimeDirName = () => {
  const packageJson = readRootPackageJsonSync();
  const productName = ifKfDev()
    ? 'electron'
    : packageJson.kungfuCraft?.productName || 'Kungfu';

  switch (os.platform()) {
    case 'win32':
    case 'linux':
      return productName;
    case 'darwin':
      return productName + '.app';
    default:
      return productName;
  }
};

export const getChildFileStat = async (
  dirname: string,
): Promise<Array<{ childFilePath: string; stat: Stats }>> => {
  if (!(await fse.pathExists(dirname))) {
    return [];
  }

  const cDirs = await fse.readdir(dirname);
  const statsDatas: Array<{ childFilePath: string; stat: Stats }> =
    await Promise.all(
      cDirs.map((cDir: string) => {
        const childFilePath = path.join(dirname, cDir);
        return fse.stat(childFilePath).then((stat: Stats) => {
          return {
            childFilePath,
            stat,
          };
        });
      }),
    );

  return statsDatas;
};

export const removeNoDefaultStrategyFolders = async (): Promise<void> => {
  const strategyDirs = RuntimeChildDirTypes.map((type) =>
    path.join(buildRuntimeChildDirByType(type), 'strategy'),
  );
  for (const strategyDir of strategyDirs) {
    if (!fse.pathExists(strategyDir)) continue;
    const filedirList: string[] = (await listDir(strategyDir)) || [];
    filedirList.map((fileOrFolder) => {
      const fullPath = path.join(strategyDir, fileOrFolder);
      if (fileOrFolder === 'default') {
        if (fse.statSync(fullPath).isDirectory()) {
          return Promise.resolve();
        }
      }
      return fse.remove(fullPath);
    });
  }
};
