import { defineStore } from 'pinia';

interface journalState {
  sessions: KungfuApi.Session[];
  currentSessionFrames: KungfuApi.FrameResolved[];

  currentSession: KungfuApi.SessionResolved | null;
}

export const useJournalStore = defineStore('journal', {
  state: (): journalState => ({
    sessions: [],
    currentSessionFrames: [],
    currentSession: null,
  }),
  actions: {
    setSessions(sessions: KungfuApi.Session[]) {
      this.sessions = sessions;
    },
    setCurrentSessionFrames(frames: KungfuApi.FrameResolved[]) {
      this.currentSessionFrames = frames;
    },
    setCurrentSession(session: KungfuApi.SessionResolved) {
      this.currentSession = session;
    },
  },
});
