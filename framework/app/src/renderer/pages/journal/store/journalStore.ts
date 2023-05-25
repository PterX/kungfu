import { defineStore } from 'pinia';

interface journalState {
  sessions: KungfuApi.Session[];
  currentSession: KungfuApi.SessionResolved | null;
}
export const useJournalStore = defineStore('journal', {
  state: (): journalState => ({
    sessions: [],
    currentSession: null,
  }),
  actions: {
    setSessions(sessions: KungfuApi.Session[]) {
      this.sessions = sessions;
    },

    setCurrentSession(session: KungfuApi.SessionResolved) {
      this.currentSession = session;
    },
  },
});
