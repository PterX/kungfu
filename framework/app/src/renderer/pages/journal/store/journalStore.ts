import { defineStore } from 'pinia';

interface journalState {
  sessions: KungfuApi.Session[];
  currentSessionFrames: KungfuApi.FrameResolved[];

  isSessionFramesInit: boolean;
  lastUpdateSessionFrames: KungfuApi.FrameResolved[];

  currentSession: KungfuApi.SessionResolved | null;
}

export const useJournalStore = defineStore('journal', {
  state: (): journalState => ({
    sessions: [],
    currentSessionFrames: [],
    isSessionFramesInit: true,
    lastUpdateSessionFrames: [],
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
    setCurrentSession(session: KungfuApi.SessionResolved) {
      this.currentSession = session;
    },
  },
});
