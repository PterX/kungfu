import { ipcRenderer } from 'electron';
import { getCurrentWindow, BrowserWindow } from '@electron/remote';
const currentWin = getCurrentWindow();
const paWin = currentWin.getParentWindow();

ipcRenderer.setMaxListeners(10);

export const ipcEmitDataByName = (
  name: string,
  postData?: {
    codeId: string;
    fileNewPath?: string;
  },
  interval?: number,
): Promise<{ data: unknown }> => {
  interval = interval || 5000;
  if (!paWin) {
    return Promise.reject(new Error('paWin is null!'));
  }

  return new Promise((resolve, reject) => {
    paWin.webContents.send(`ipc-emit-${name}`, {
      childWinId: currentWin.id,
      params: Object.freeze(postData || {}),
    });
    const timer = setTimeout(() => {
      reject(new Error(`ipc-emit-${name} timeout`));
      ipcRenderer.removeAllListeners(`ipc-res-${name}`);
      clearTimeout(timer);
    }, interval);

    ipcRenderer.on(`ipc-res-${name}`, (event, data) => {
      resolve({ data });
      ipcRenderer.removeAllListeners(`ipc-res-${name}`);
      clearTimeout(timer);
    });
  });
};

export const ipcEmit = (
  name: string,
  params: Record<string, string>,
): Promise<void> => {
  return new Promise((resolve) => {
    const allWindows = BrowserWindow.getAllWindows();
    allWindows.forEach((win) => {
      win.webContents.send(name, params);
    });
    resolve();
  });
};
