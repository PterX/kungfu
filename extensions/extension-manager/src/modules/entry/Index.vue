<template>
  <build-outlined style="font-size: 24px" />
</template>

<script lang="ts" setup>
import { BuildOutlined } from '@ant-design/icons-vue';
import { useExtManagerStore } from '../../store';

const extManagerStore = useExtManagerStore();

extManagerStore.setAllExtensions();
</script>

<style lang="less"></style>
