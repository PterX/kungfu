import './setEnv';
import './injectWindow';
import { createApp } from 'vue';
import App from '@kungfu-trader/kungfu-app/src/renderer/pages/journal/App.vue';

import store from './store';
import VueVirtualScroller from 'vue-virtual-scroller';
import { dragging } from './directives';
import 'vue-virtual-scroller/dist/vue-virtual-scroller.css';
import {
  Button,
  Checkbox,
  Drawer,
  Empty,
  Input,
  Layout,
  Spin,
  Slider,
  List,
  Card,
  Tree,
  Modal,
  Form,
  Select,
  Menu,
  Tag,
} from 'ant-design-vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';

const app = createApp(App);

app
  .use(store)
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
  .use(Menu)
  .use(Slider)
  .use(Tag)
  .use(VueVirtualScroller);

app.use(VueI18n);
app.directive('dragging', dragging);
app.mount('#app');
