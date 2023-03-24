import { ipcRenderer } from 'electron';
import {
  getCodeInfoById,
  updateCurrentCodePath,
} from '@kungfu-trader/kungfu-js-api/kungfu/strategy';
import { BrowserWindow } from '@electron/remote';
import { messagePrompt } from '../assets/methods/uiUtils';
const { success, error } = messagePrompt();

export function bindIPCListener(store) {
  ipcRenderer.removeAllListeners('ipc-emit-CodeById');
  ipcRenderer.on('ipc-emit-CodeById', (event, { childWinId, params }) => {
    const childWin = BrowserWindow.fromId(childWinId);
    const { codeId } = params;
    return getCodeInfoById(codeId)
      .then((strategies) => {
        if (childWin) {
          childWin.webContents.send('ipc-res-CodeById', strategies);
        }
      })
      .catch((err) => {
        error(err.message);
      });
  });

  ipcRenderer.removeAllListeners('ipc-emit-updateCurrentCodePath');
  ipcRenderer.on(
    'ipc-emit-updateCurrentCodePath',
    (event, { childWinId, params }) => {
      const childWin = BrowserWindow.fromId(childWinId);
      const { codeId, fileNewPath } = params;
      return updateCurrentCodePath(codeId, fileNewPath).then(() => {
        store.setKfConfigList();
        success();
        if (childWin) {
          childWin.webContents.send('ipc-res-updateCurrentCodePath');
        }
      });
    },
  );

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
}
