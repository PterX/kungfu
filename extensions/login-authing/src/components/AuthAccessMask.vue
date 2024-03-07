<template>
  <div v-if="noAccessState" :id="id" class="kf-authing-access-mask">
    <div class="access-mask-content__wrapper">
      <div
        class="access-mask-content"
        :style="{
          marginTop,
        }"
      >
        <template v-if="noAccessState === StatesEnum.notLogin">
          <span>{{ $t('loginAuthing.notLoginTip') }}</span>
          ,
          <a @click="handleToLogin">{{ $t('loginAuthing.notLogin') }}</a>
        </template>
        <template v-if="noAccessState === StatesEnum.noAccess">
          <a @click="handleToConcatUs">
            {{ $t('loginAuthing.concatUs') }}
          </a>
          ,
          <span>
            {{
              $t('loginAuthing.applyForUse', {
                name,
              })
            }}
          </span>
        </template>
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
import {
  ref,
  computed,
  getCurrentInstance,
  onMounted,
  nextTick,
  watchEffect,
} from 'vue';

import VueI18n, { useLanguage } from '@kungfu-trader/kungfu-js-api/language';
import { useAuthingCredential } from '../utils/externalUtils';
import { LoginAuthingKeys } from '../configs';

const locals = {
  'zh-CN': {
    loginAuthing: {
      notLogin: '请先登录',
      notLoginTip: '还未登录',
      concatUs: '联系我们',
      applyForUse: '申请试用{name}',
    },
  },
  'en-US': {
    loginAuthing: {
      notLogin: 'Please login first',
      notLoginTip: 'Not logged in',
      concatUs: 'Connect us',
      applyForUse: 'apply for trial {name}',
    },
  },
};

const { isLanguageKeyAvailable } = useLanguage();

if (!isLanguageKeyAvailable('loginAuthing.applyForUse')) {
  Object.keys(locals).forEach((lang) => {
    VueI18n.global.mergeLocaleMessage(lang, locals[lang]);
  });
}

withDefaults(
  defineProps<{
    access?: string[];
    name?: string;
    marginTop?: string;
  }>(),
  { access: () => [], name: '', marginTop: '16%' },
);

enum StatesEnum {
  hasAccess = 0,
  notLogin = 1,
  noAccess = 2,
}

const app = getCurrentInstance();
const { getCurrentCredential } = useAuthingCredential();
const id = ref(`kf-authing-access-mask-${Date.now()}`);

const noAccessState = computed<StatesEnum>(() => {
  if (!getCurrentCredential()) return StatesEnum.notLogin;

  return StatesEnum.hasAccess;
});

let styleAdded = false;
const checkParentElStyle = () => {
  if (styleAdded) return;

  const maskEl = document.querySelector(`#${id.value}`) as HTMLElement | null;
  if (maskEl) {
    const parentEl = maskEl.parentElement;
    if (parentEl && !parentEl.style.position) {
      parentEl.style.position = 'relative';
      styleAdded = true;
    }
  }
};

watchEffect(() => {
  if (noAccessState.value) {
    nextTick(() => {
      checkParentElStyle();
    });
  }
});

onMounted(() => {
  checkParentElStyle();
});

const handleToLogin = () => {
  app?.proxy?.$globalBus.next({
    tag: LoginAuthingKeys.CallLogin,
  });
};

const handleToConcatUs = () => {
  app?.proxy?.$globalBus.next({
    tag: LoginAuthingKeys.ConcatUs,
  });
};
</script>

<style lang="less">
.kf-authing-access-mask {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 100;
  background: rgba(0, 0, 0, 0.6);
  display: flex;
  justify-content: center;

  .access-mask-content__wrapper {
    height: 100%;
    width: 100%;

    .access-mask-content {
      width: fit-content;
      margin: 15% auto;
    }
  }
}
</style>
