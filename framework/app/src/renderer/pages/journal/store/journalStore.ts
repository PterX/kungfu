import { defineStore } from 'pinia';

interface journalState {
  sessions: KungfuApi.Session[];
  currentSessionFrames: KungfuApi.FrameResolved[];
  mdSessionFrames: KungfuApi.FrameResolved[];
  isSessionFramesInit: boolean;
  isMdSessionFramesInit: boolean;
  lastUpdateSessionFrames: KungfuApi.FrameResolved[];
  mdLastUpdateSessionFrames: KungfuApi.FrameResolved[];
  currentSession: KungfuApi.SessionResolved | null;
}
export const useJournalStore = defineStore('journal', {
  state: (): journalState => ({
    sessions: [],
    currentSessionFrames: [],
    mdSessionFrames: [],
    isSessionFramesInit: true,
    isMdSessionFramesInit: true,
    lastUpdateSessionFrames: [],
    mdLastUpdateSessionFrames: [],
    currentSession: null,
  }),
  actions: {
    setSessions(sessions: KungfuApi.Session[]) {
      this.sessions = sessions;
    },
    setCurrentSessionFrames(
      frames: KungfuApi.FrameResolved[],
      isInit: boolean,
    ) {
      if (isInit) {
        this.currentSessionFrames = frames;
      } else {
        this.currentSessionFrames.push(...frames);
      }

      this.isSessionFramesInit = isInit;
      this.lastUpdateSessionFrames = frames;
    },
    setMdSessionFrames(frames: KungfuApi.FrameResolved[], isInit: boolean) {
      if (isInit) {
        this.mdSessionFrames = frames;
      } else {
        this.mdSessionFrames.push(...frames);
      }

      this.isMdSessionFramesInit = isInit;
      this.mdLastUpdateSessionFrames = frames;
    },
    setCurrentSession(session: KungfuApi.SessionResolved) {
      this.currentSession = session;
    },
  },
});
