import { ipcRenderer } from 'electron';
import { BrowserWindow } from '@electron/remote';
import { startReplay } from '@kungfu-trader/kungfu-js-api/utils/processUtils';

export function bindIPCListener(store) {
  ipcRenderer.removeAllListeners('ipc-emit-strategyList');
  ipcRenderer.on('ipc-emit-strategyList', (event, { childWinId }) => {
    const childWin = BrowserWindow.fromId(childWinId);
    return new Promise(() => {
      if (childWin) {
        childWin.webContents.send('ipc-res-strategyList', store.strategyList);
      }
    });
  });

  ipcRenderer.removeAllListeners('ipc-emit-strategyStates');
  ipcRenderer.on('ipc-emit-strategyStates', (event, { childWinId }) => {
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
  ipcRenderer.on('startReplay', async (_event, args) => {
    const { replayProcessParams } = args;
    const { category, group, name, replayConfig } = replayProcessParams;
    const location: KungfuApi.KfLocation = {
      category,
      group,
      name,
      mode: 'replay',
    };
    await startReplay(location, replayConfig);
  });
}
