<script setup lang="ts">
import { onMounted } from 'vue';

onMounted(() => {
  console.log(22222, '````````````````````');
});
</script>

<template>
  <div>
    <p>测试页面111111111111</p>
  </div>
</template>

<style lang="less">
#app {
  width: 100%;
  height: 100%;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;

  .app__warp {
    height: 100%;
    width: 100%;
  }
}
</style>
