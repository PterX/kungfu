import { defineStore } from 'pinia';
import {
  dealCategory,
  getCurrentLocation,
  dealSession,
  getNowInNano,
} from '../utils';
import { JournalLoadingtype } from '../types';
import { SessionStatusEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { getAllSessions } from '@kungfu-trader/kungfu-js-api/kungfu/store';

interface journalState {
  sessions: KungfuApi.SessionResolved[];
  currentSession: KungfuApi.SessionResolved | null;
  currentFrameList: KungfuApi.FrameResolved[];
  currentTime: bigint;
  currentLoadedLastestFrameGenTime: bigint;
  currentLocation: KungfuApi.KfExtractLocation | null;
  journalLoadingType: JournalLoadingtype;
  isBuildingTracer: boolean;
  selectedChartItem: number;
  currentFrame: KungfuApi.FrameResolved | null;
  currentFrameId: string;
}

export const useJournalStore = defineStore('journal', {
  state: (): journalState => ({
    sessions: [],
    currentSession: null,
    currentFrameList: [],
    currentTime: getNowInNano(),
    currentLoadedLastestFrameGenTime: 0n,
    currentLocation: null,
    journalLoadingType: 'finish',
    isBuildingTracer: false,
    selectedChartItem: 0,
    currentFrame: null,
    currentFrameId: '',
  }),

  actions: {
    setCurrentLocation() {
      const location = getCurrentLocation();
      this.currentLocation = location;
      return location;
    },

    setCurrentSession(currentSession: KungfuApi.SessionResolved) {
      this.currentSession = currentSession;
      this.currentTime = this.currentSession.begin_time;
      this.currentLoadedLastestFrameGenTime = this.currentTime;
    },

    async setSessions() {
      const location = this.setCurrentLocation();
      const sessions = await getAllSessions(location);
      this.sessions = sessions
        .sort((a, b) => Number(b.begin_time) - Number(a.begin_time))
        .map((item) => dealSession(item));

      if (this.sessions.length !== 0) {
        if (this.currentSession === null) {
          this.setCurrentSession(this.sessions[0]);
        } else {
          const findNewCurrentSession = this.sessions.find(
            (item) => item.begin_time === this.currentSession?.begin_time,
          );
          if (findNewCurrentSession) {
            this.setCurrentSession(findNewCurrentSession);
          } else {
            this.setCurrentSession(this.sessions[0]);
          }
        }
      }
    },

    setCurrentTime(nano: bigint) {
      this.currentTime !== nano ? (this.currentTime = nano) : null;
    },

    setCurrentFrameList(frameList: KungfuApi.FrameResolved[]) {
      this.currentFrameList = frameList;
    },

    setCurrentLastFrameTime(nano: bigint) {
      this.currentLoadedLastestFrameGenTime = nano;
    },

    setSelectedChartItem(index: number) {
      this.selectedChartItem = index;
    },

    setCurrentFrameId(id: string) {
      this.currentFrameId = id;
    },

    setCurrentFrame(frame: KungfuApi.FrameResolved) {
      this.currentFrame = frame;
    },
  },

  getters: {
    currentSessionKey(): bigint {
      return this.currentSession ? this.currentSession.begin_time : 0n;
    },

    currentSessionName(): string {
      return this.currentSession ? this.currentSession.sessionName : '';
    },

    currentCategoryData(): KungfuApi.KfTradeValueCommonData {
      return dealCategory(this.currentSession?.category);
    },

    currentSessionBeginTime(): bigint {
      return this.currentSession ? this.currentSession.begin_time : 0n;
    },

    currentSessionEndTime(): bigint {
      return this.currentSession
        ? this.currentSession.status === SessionStatusEnum.Finished
          ? this.currentSession.end_time
          : 0n
        : 0n;
    },
  },
});
