<template>
  <div class="kf-no-data__warp">
    <div class="kf-no-data__content">
      <img svg-inline :src="logo" alt="" />
      <p>{{ txt }}</p>
    </div>
  </div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import {
  isNoDataDefaultLogo,
  getLogoNoBgPath,
} from '@kungfu-trader/kungfu-js-api/config/brand';

export default defineComponent({
  props: {
    txt: {
      type: String,
    },
  },

  setup() {
    return {
      logo: isNoDataDefaultLogo()
        ? require('@kungfu-trader/kungfu-app/src/renderer/assets/svg/LOGO-NO-BG.svg')
        : getLogoNoBgPath(),
    };
  },
});
</script>

<style lang="less">
.kf-no-data__warp {
  height: 100%;
  width: 100%;
  .kf-no-data__content {
    img {
      height: 88px;
      width: 88px;
      display: block;
      margin: auto;
      margin-top: 20px;
      filter: invert(0) sepia(0%) saturate(0%) hue-rotate(0) brightness(32%)
        contrast(129%);
    }

    p {
      text-align: center;
      padding-top: 4px;
      color: @text-color-secondary;
    }
  }
}
</style>
