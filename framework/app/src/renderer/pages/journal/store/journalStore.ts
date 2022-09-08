import { defineStore } from 'pinia';

interface journalState {
  sessions: KungfuApi.Session[];
  frames: KungfuApi.Frame[];

  currentSession: KungfuApi.SessionResolved | null;
}

export const useJournalStorage = defineStore('journal', {
  state: (): journalState => ({
    sessions: [],
    frames: [],
    currentSession: null,
  }),
  actions: {
    setSessions(sessions: KungfuApi.Session[]) {
      this.sessions = sessions;
    },
    setFrames(frames: KungfuApi.Frame[]) {
      this.frames = frames;
    },
    setCurrentSession(session: KungfuApi.SessionResolved) {
      this.currentSession = session;
    },
  },
});
