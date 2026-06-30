import {
  defineComponent,
  h,
  getCurrentInstance,
  onMounted,
  ref,
  watch,
} from 'vue';
import { BulbOutlined } from '@ant-design/icons-vue';
import VueI18n from '@kungfu-tech/api/language';

import { myDriver } from './myDriver';

export default defineComponent({
  name: 'StartNoviceGuide',
  setup() {
    const { t } = VueI18n.global;
    const app = getCurrentInstance();
    const visible = ref(true);

    onMounted(() => {
      watch(
        () => app?.proxy?.$route,
        (newVal) => {
          visible.value = newVal?.path === '/main';
        },
      );
    });

    return () =>
      visible.value
        ? h(
            'div',
            {
              onClick: () => myDriver.drive(0, true),
            },
            [
              h(BulbOutlined),
              h(
                'span',
                { style: 'margin-left: 4px' },
                t('noviceGuide.noviceGuide'),
              ),
            ],
          )
        : null;
  },
});
