import { ipcRenderer } from 'electron';
import { BrowserWindow } from '@electron/remote';

export function bindIPCListener(store) {
  ipcRenderer.removeAllListeners('ipc-emit-strategyList');
  ipcRenderer.on('ipc-emit-strategyList', (_event, { childWinId }) => {
    const childWin = BrowserWindow.fromId(childWinId);
    return new Promise(() => {
      if (childWin) {
        childWin.webContents.send('ipc-res-strategyList', store.strategyList);
      }
    });
  });

  ipcRenderer.removeAllListeners('ipc-emit-strategyStates');
  ipcRenderer.on('ipc-emit-strategyStates', (_event, { childWinId }) => {
    const childWin = BrowserWindow.fromId(childWinId);
    return new Promise(() => {
      if (childWin) {
        childWin.webContents.send(
          'ipc-res-strategyStates',
          store.strategyStates,
        );
      }
    });
  });
}
