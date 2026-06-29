import {
  killExtra,
  pm2Kill,
  pm2KillGodDaemon,
} from '@kungfu-tech/api/utils/processUtils';
import 'console-success';
import { deleteNNFiles } from '@kungfu-tech/api/utils/busiUtils';
import { delayMilliSeconds } from '@kungfu-tech/api/utils/commonUtils';

export const shutdown = async () => {
  try {
    console.log('Killing all Process ...');
    await pm2Kill();
    console.log('Killing pm2 ...');
    await pm2KillGodDaemon();
    console.log('Killing extra ...');
    await killExtra(true);
    await delayMilliSeconds(1000);
    console.log('Deleting socket files ...');
    await deleteNNFiles();

    console.success(`Shutdown kungfu`);
    process.exit(0);
  } catch (err) {
    console.error(err);
    process.exit(1);
  }
};
