import { createRouter, createWebHashHistory, RouteRecordRaw } from 'vue-router';
import Index from '@kungfu-trader/kungfu-app/src/renderer/pages/index/views/Index.vue';
import Test from '@kungfu-trader/kungfu-app/src/renderer/pages/index/views/Test.vue';

const routes: Array<RouteRecordRaw> = [
  {
    path: '/index',
    name: 'Index',
    component: Index,
    meta: {
      keepAlive: true,
    },
  },
  {
    path: '/test',
    name: 'Test',
    component: Test,
    meta: {
      keepAlive: true,
    },
  },
];

const router = createRouter({
  history: createWebHashHistory(),
  routes,
});

export default router;
