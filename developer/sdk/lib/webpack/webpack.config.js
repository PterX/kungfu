'use strict';

const toolkit = require('@kungfu-tech/api/toolkit');
const path = require('path');
const { merge } = require('webpack-merge');
const CopyPlugin = require('copy-webpack-plugin');
const {
  getWebpackExternals,
  getSdkDefaultDistDir,
} = require('@kungfu-tech/api/toolkit/utils');
const webpack = require('webpack');

const webpackConfig = (argv) => {
  argv.passTJSLoader = true;
  return merge(toolkit.webpack.makeConfig(argv), {
    externals: getWebpackExternals(),
    mode: 'production',
    entry: {
      kfs: path.join(__dirname, '..', '..', 'src', 'kfs.js'),
    },
    module: {
      rules: [
        {
          test: /\.(t|j)s$/,
          include: [path.join(__dirname, '..', '..', 'src')],
          use: [
            {
              loader: 'babel-loader',
            },
          ],
        },
      ],
    },
    resolve: {
      alias: {
        '@kungfu-tech/api': path.join(
          require.resolve('@kungfu-tech/api'),
          '../../',
        ),
      },
    },
    target: 'node',
    plugins: [
      new webpack.IgnorePlugin({
        resourceRegExp: /kungfu-cli/,
      }),
      new webpack.IgnorePlugin({
        resourceRegExp: /kungfu-app/,
      }),
      new CopyPlugin({
        patterns: [
          {
            from: path.join(__dirname, '..', '..', 'templates'),
            to: path.join(getSdkDefaultDistDir(), 'sdk', 'templates'),
            context: path.resolve(getSdkDefaultDistDir(), 'sdk'),
          },
        ],
      }),
    ],
  });
};

module.exports = (argv) => {
  return webpackConfig(argv);
};
