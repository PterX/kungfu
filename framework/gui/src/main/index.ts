// Minimal Electron main process for the kungfu reference app.
//
// The C++ runtime reads configuration through std::getenv, which only sees
// environment variables present when the process starts. The renderer process
// is spawned by this main process, so the runtime directory must be exported
// here, before any window (and therefore the renderer process) is created.
import { app, BrowserWindow } from 'electron';
import path from 'node:path';

// Resolve the kungfu runtime directory (kfc) that holds libkungfu.dylib and the
// kungfu_electron.node binding. In development it lives in the kungfu-core
// package; once packaged it is shipped as an extraResource under Resources/kfc.
const kfcDir = app.isPackaged
  ? path.join(process.resourcesPath, 'kfc')
  : path.join(
      path.dirname(require.resolve('@kungfu-tech/core/package.json')),
      'dist',
      'kfc',
    );

const bindingPath = path.join(kfcDir, 'kungfu_electron.node');

// Export before the renderer process is created so both processes inherit them.
process.env.KF_RUNTIME_DIR = process.env.KF_RUNTIME_DIR || kfcDir;
process.env.KFE_PATH = process.env.KFE_PATH || bindingPath;

// Probe the binding in the main (node) process.
try {
  const kfe = require(bindingPath);
  console.log(
    `KFE_MAIN_OK loaded; exports=${Object.keys(kfe).length} [${Object.keys(kfe)
      .slice(0, 6)
      .join(',')}]`,
  );
} catch (e) {
  console.log(`KFE_MAIN_FAIL ${(e as Error).message}`);
}

function createWindow() {
  const win = new BrowserWindow({
    width: 1280,
    height: 800,
    show: false,
    backgroundColor: '#1e1e1e',
    webPreferences: {
      // Moat: in-process zero-copy access to journal/state requires
      // nodeIntegration with contextIsolation/sandbox disabled.
      nodeIntegration: true,
      contextIsolation: false,
      sandbox: false,
    },
  });

  win.on('ready-to-show', () => win.show());

  if (process.env.ELECTRON_RENDERER_URL) {
    win.loadURL(process.env.ELECTRON_RENDERER_URL);
  } else {
    win.loadFile(path.join(__dirname, '../renderer/index.html'));
  }
}

app.whenReady().then(createWindow);

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) createWindow();
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});
