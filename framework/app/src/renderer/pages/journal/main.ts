import './setEnv';
import { createApp } from 'vue';
import App from '@kungfu-trader/kungfu-app/src/renderer/pages/journal/App.vue';

import VueVirtualScroller from 'vue-virtual-scroller';
import 'vue-virtual-scroller/dist/vue-virtual-scroller.css';
import {
  Button,
  Checkbox,
  Drawer,
  Empty,
  Input,
  Layout,
  Spin,
  List,
  Card,
  Tree,
  Modal,
  Form,
  Select,
} from 'ant-design-vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const app = createApp(App);

app
  .use(Layout)
  .use(Button)
  .use(Checkbox)
  .use(Input)
  .use(Spin)
  .use(Drawer)
  .use(Empty)
  .use(List)
  .use(Card)
  .use(Tree)
  .use(Modal)
  .use(Select)
  .use(Form)
  .use(VueVirtualScroller);

app.use(VueI18n);
app.mount('#app');
