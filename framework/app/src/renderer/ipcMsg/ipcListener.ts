import { ipcRenderer } from 'electron';
import { BrowserWindow } from '@electron/remote';
import {
  stopProcess,
  listProcessStatus,
} from '@kungfu-trader/kungfu-js-api/utils/processUtils';
import { startReplay } from '@kungfu-trader/kungfu-js-api/utils/busiUtils';
import { getProcessIdByKfLocation } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';

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
  ipcRenderer.on('startReplay', async (_event, args) => {
    const { replayProcessParams } = args;
    const { category, group, name, mode, replayConfig } = replayProcessParams;
    const processId = getProcessIdByKfLocation({
      category,
      group,
      name,
      mode,
    });
    const { processStatus } = await listProcessStatus();
    if (processStatus[processId] === 'online') {
      await stopProcess(processId);
    }
    await startReplay(
      {
        category,
        group,
        name,
        mode: 'live',
      },
      replayConfig,
    );
  });
}
