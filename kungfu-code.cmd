@echo off
rem kungfu-code.cmd — kungfu 开发/构建编排「起手式」(Windows),三层子集的 L1。
rem 与 macOS/Linux 的 kungfu-code(sh)对齐:
rem   kungfu-code app | kungfu-code build:core | kungfu-code <任意 yarn 任务>
rem   kungfu-code proxy ...   富子命令:管理本地缓存/镜像代理配置(委派 L2 node,不透传 yarn)
rem
rem 三层子集:L1 本文件(.cmd)bootstrap+委派;L2 kungfu-code.js(node)富命令;L3(未来)TUI。
rem 一次性装两个前置:fnm(winget install Schniz.fnm)+ uv(winget install astral-sh.uv)。
rem 本仓零 LAN/镜像耦合(开源者克隆即用官方上游)。需局域网缓存/CN 镜像时,用
rem `kungfu-code config` 把模板 build-local.env.example 派生到用户全局并填值:
rem   %USERPROFILE%\.config\kungfu\build-local.env  (sh 格式,主仓与所有 worktree 共用)
rem 本 .cmd 以纯 cmd 解析该文件加载镜像 env;可选仓根 .\build-local.env 叠加覆盖。
setlocal enabledelayedexpansion
cd /d "%~dp0"

rem ── 富子命令委派 L2 node(不透传 yarn、不需要 uv)。有 fnm 优先 fnm-managed node,否则回退系统 node ──
if /i "%~1"=="proxy"  goto delegate
if /i "%~1"=="config" goto delegate
goto bootstrap

:delegate
where fnm >nul 2>nul && (
  fnm install >nul 2>nul
  fnm exec --using-file -- node "%~dp0kungfu-code.js" %*
  exit /b !errorlevel!
)
where node >nul 2>nul && (
  node "%~dp0kungfu-code.js" %*
  exit /b !errorlevel!
)
echo kungfu-code: 富子命令需 node —— 装 fnm(winget install Schniz.fnm)或任一系统 node 均可 1>&2
exit /b 127

:bootstrap
rem 加载本地缓存代理配置:用户全局优先,再叠加可选仓内覆盖(set 后随进程透传子进程)。
set "_KFC_USERCFG=%USERPROFILE%\.config\kungfu\build-local.env"
if defined XDG_CONFIG_HOME set "_KFC_USERCFG=%XDG_CONFIG_HOME%\kungfu\build-local.env"
call :loadenv "%_KFC_USERCFG%"
call :loadenv ".\build-local.env"

where fnm >nul 2>nul || (
  echo kungfu-code: 需先一次性安装 fnm(node 侧前置)—— winget install Schniz.fnm ^(或见 https://github.com/Schniz/fnm^) 1>&2
  exit /b 127
)
where uv >nul 2>nul || (
  echo kungfu-code: 需先一次性安装 uv(python 侧前置)—— winget install astral-sh.uv ^(或见 https://docs.astral.sh/uv/^) 1>&2
  exit /b 127
)

rem 幂等:确保 .node-version 指定的 node 已安装
fnm install >nul 2>nul

rem 在钉定 node 下,经 corepack 跑 packageManager 钉定的 yarn
fnm exec --using-file -- corepack yarn %*
exit /b !errorlevel!

rem ── 纯 cmd 解析 sh 格式 build-local.env 的 `export KEY='VALUE'` 行 → set KEY=VALUE ──
rem (Windows cmd 无法 source sh;此处只取 export 行、剥 export 前缀与单引号;URL 无内嵌引号/等号问题)
:loadenv
if not exist "%~1" goto :eof
for /f "usebackq tokens=1,* delims==" %%a in (`findstr /b /c:"export " "%~1"`) do (
  set "_kfc_k=%%a"
  set "_kfc_v=%%b"
  set "_kfc_k=!_kfc_k:export =!"
  set "_kfc_v=!_kfc_v:'=!"
  set "!_kfc_k!=!_kfc_v!"
)
set "_kfc_k="
set "_kfc_v="
goto :eof
