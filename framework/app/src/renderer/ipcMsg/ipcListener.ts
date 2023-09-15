import { ipcRenderer } from 'electron';
import { BrowserWindow } from '@electron/remote';
import {
  stopProcess,
  listProcessStatus,
} from '@kungfu-trader/kungfu-js-api/utils/processUtils';
import {
  getProcessIdByKfLocation,
  startReplay,
} from '@kungfu-trader/kungfu-js-api/utils/busiUtils';

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
    const processId = getProcessIdByKfLocation(
      {
        category,
        group,
        name,
        mode: 'replay',
      },
      'replay',
    );
    const { processStatus } = await listProcessStatus();
    if (processStatus[processId]) {
      await stopProcess(processId);
    }
    await startReplay(
      {
        category,
        group,
        name,
        mode: 'replay',
      },
      replayConfig,
    );
  });
}
