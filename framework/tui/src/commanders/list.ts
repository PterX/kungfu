import { getAllKfConfigOriginData } from '@kungfu-tech/api/actions';
import { getKfExtensionConfig } from '@kungfu-tech/api/utils/extUtils';
import { getIdByKfLocation } from '@kungfu-tech/api/utils/commonUtils';
import colors from 'colors';
import { dealKfConfigValue, parseToString } from '../assets/methods/utils';

export const listKfLocations = async (): Promise<void> => {
  const { md, td, strategy, operator } = await getAllKfConfigOriginData();
  const extConfigs = await getKfExtensionConfig();

  const mdList = await Promise.all(
    md.map(async (m): Promise<string> => {
      return parseToString(
        [
          colors.yellow('Md'),
          colors.bold(getIdByKfLocation(m)),
          await dealKfConfigValue(m, extConfigs),
        ],
        [10, 15, 'auto'],
        1,
      );
    }),
  );

  const tdList = await Promise.all(
    td.map(async (t): Promise<string> => {
      return parseToString(
        [
          colors.cyan('Td'),
          colors.bold(getIdByKfLocation(t)),
          await dealKfConfigValue(t, extConfigs),
        ],
        [10, 15, 'auto'],
        1,
      );
    }),
  );
  const strategyList = strategy.map((s): string => {
    return parseToString(
      [
        colors.blue('strategy'),
        colors.bold(getIdByKfLocation(s)),
        JSON.parse(s.value).file_path,
      ],
      [10, 15, 'auto'],
      1,
    );
  });

  const operatorList = await Promise.all(
    operator.map(async (s): Promise<string> => {
      return parseToString(
        [
          colors.green('operator'),
          colors.bold(getIdByKfLocation(s)),
          s.group === 'default'
            ? JSON.parse(s.value).file_path
            : await dealKfConfigValue(s, extConfigs),
        ],
        [10, 15, 'auto'],
        1,
      );
    }),
  );

  if ([...mdList, ...tdList, ...strategyList, ...operatorList].length === 0) {
    console.log('No mds, tds, strategies and operators');
  } else {
    console.log(
      [
        `\n${colors.bold.underline('Md:')} \n`,
        ...mdList,
        `\n${colors.bold.underline('Td:')} \n`,
        ...tdList,
        `\n${colors.bold.underline('Strategy:')} \n`,
        ...strategyList,
        `\n${colors.bold.underline('Operator:')} \n`,
        ...operatorList,
      ].join('\n'),
    );
  }
};
