@echo off
rem kungfu-code — kungfu 开发/构建编排「起手式」(Windows)
rem 用法:kungfu-code app | kungfu-code build:core | kungfu-code <任意 yarn 任务>
rem 机制同 macOS/Linux 版:fnm 选 node(.node-version)→ corepack 派发 yarn。
rem 使用者只需一次性安装 fnm(winget install Schniz.fnm)。
setlocal
cd /d "%~dp0"
if not defined FNM_NODE_DIST_MIRROR set "FNM_NODE_DIST_MIRROR=http://192.168.100.222:8088/node/"
if not defined COREPACK_NPM_REGISTRY set "COREPACK_NPM_REGISTRY=http://192.168.100.222:4873"
if not defined ELECTRON_MIRROR set "ELECTRON_MIRROR=http://192.168.100.222:8088/electron/"
where fnm >nul 2>nul || (
  echo kungfu-code: 需先一次性安装 fnm —— winget install Schniz.fnm ^(或见 https://github.com/Schniz/fnm^)
  exit /b 127
)
fnm install >nul 2>nul
fnm exec --using-file -- corepack yarn %*
