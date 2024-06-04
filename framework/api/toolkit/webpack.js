const path = require('path');
const webpack = require('webpack');
const TerserPlugin = require('terser-webpack-plugin');
const ESLintPlugin = require('eslint-webpack-plugin');
const ForkTsCheckerWebpackPlugin = require('fork-ts-checker-webpack-plugin');
const UnpluginIcons = require('unplugin-icons/webpack').default;
const { isProduction, getAppDir } = require('./utils');

module.exports = {
  getThreadLoaderConfig(argv) {
    const threadsNum = argv.threadsNum ?? 6;
    return typeof threadsNum === 'number' && threadsNum !== 1
      ? [
          {
            loader: 'thread-loader',
            options: {
              workers: threadsNum,
              workerParallelJobs: 50,
              workerNodeArgs: ['--max-old-space-size=1024'],
              poolRespawn: false,
              poolTimeout: isProduction(argv) ? 5000 : Infinity,
              name: 'kungfu-webpack-pool',
            },
          },
        ]
      : [];
  },
  makeConfig(argv) {
    const production = isProduction(argv);
    console.log('enableThreadLoader', argv.enableThreadLoader);
    const threadLoader = argv.enableThreadLoader
      ? this.getThreadLoaderConfig(argv)
      : [];
    const tjsIncludes = {
      ...(argv.tjsIncludes ? { include: argv.tjsIncludes } : {}),
      ...(argv.tjsExcludeNodeModules ?? true
        ? { exclude: /node_modules/ }
        : {}),
    };

    return {
      devtool: 'eval-source-map',
      experiments: {
        topLevelAwait: true,
      },
      mode: production ? 'production' : 'development',
      optimization: {
        minimize: true,
        minimizer: [
          new TerserPlugin({
            parallel: true,
          }),
        ],
      },
      cache: {
        type: 'filesystem',
        buildDependencies: {
          config: [__filename],
        },
      },
      module: {
        rules: [
          ...(argv.passTJSLoader
            ? []
            : [
                {
                  test: /\.[tj]s$/,
                  ...tjsIncludes,
                  use: [
                    ...threadLoader,
                    {
                      loader: 'babel-loader',
                    },
                  ],
                },
                {
                  test: /\.[tj]s$/,
                  ...tjsIncludes,
                  use: [
                    // ...threadLoader, // ts-loader 开了多线程编译会变慢
                    {
                      loader: 'ts-loader',
                      options: {
                        configFile: path.resolve(
                          process.cwd().toString(),
                          'tsconfig.json',
                        ),
                        // 对应文件添加个.ts或.tsx后缀
                        // NOTE: 这里对 vue 做了额外处理, 所以如果启用了 threadLoader, 在 vue-loader 中也需要启用 threadLoader, 否则会有问题
                        appendTsSuffixTo: ['\\.vue$'],
                        allowTsInNodeModules: true,
                        happyPackMode: !!threadLoader.length, // 无论 ts-loader 用不用多线程, 只要别处有用, 这个都得启用
                      },
                    },
                  ],
                },
              ]),
          {
            test: /\.node$/,
            use: 'node-loader',
          },
          {
            test: /\.(m?js|node)$/,
            parser: {
              amd: false,
            },
            use: {
              loader: '@vercel/webpack-asset-relocator-loader',
              options: {
                outputAssetBase: 'native_modules',
                production: production,
              },
            },
          },
          {
            test: /\.(png|jpe?g|gif|svg)(\?.*)?$/,
            use: {
              loader: 'url-loader',
              options: {
                limit: 10000,
                name: 'imgs/[name]--[folder].[ext]',
                esModule: false,
              },
            },
          },
          {
            test: /\.(mp4|webm|ogg|mp3|wav|flac|aac)(\?.*)?$/,
            use: {
              loader: 'url-loader',
              options: {
                limit: 10000,
                name: 'media/[name]--[folder].[ext]',
                esModule: false,
              },
            },
          },
        ],
      },
      node: {
        __dirname: !production,
        __filename: !production,
      },
      output: {
        globalObject: 'this',
        filename: '[name].js',
        path: path.join(argv.distDir, argv.distName),
        library: {
          type: 'commonjs2',
        },
        libraryTarget: 'commonjs2',
      },
      plugins: [
        ...(threadLoader.length // 当使用 threaderLoader 时, ts-loader 会默认只编译, 不检测类型, 需要额外引入 fork-ts-checker-webpack-plugin 来做类型检测
          ? [
              new ForkTsCheckerWebpackPlugin({
                typescript: {
                  configFile: path.resolve(
                    process.cwd().toString(),
                    'tsconfig.json',
                  ),
                  diagnosticOptions: {
                    semantic: true,
                    syntactic: true,
                  },
                },
              }),
            ]
          : []),
        new ESLintPlugin({
          fix: true /* 自动帮助修复 */,
          extensions: ['js', 'json', 'ts', 'json', 'css', 'less'],
          exclude: 'node_modules',
          failOnWarning: !production,
        }),
        new webpack.IgnorePlugin({
          checkResource(resource, context) {
            // ---- do not bundle astronomia vsop planet data
            if (/\/astronomia\/data$/.test(context)) {
              return !['./deltat.js', './vsop87Bearth.js'].includes(resource);
            }
            // ---- do not bundle moment locales
            if (/\/moment\/locale$/.test(context)) {
              return true;
            }
            return false;
          },
        }),
        UnpluginIcons(),
      ],
      resolve: {
        alias: {
          '@kungfu-trader/kungfu-js-api': path.resolve(
            path.dirname(
              path.dirname(require.resolve('@kungfu-trader/kungfu-js-api')),
            ),
            'src',
          ),
          ...(getAppDir()
            ? {
                '@kungfu-trader/kungfu-app': getAppDir(),
              }
            : {}),
        },

        extensions: ['.js', '.ts', '.d.ts', '.vue', '.json', '.css', '.node'],
      },
    };
  },
};
