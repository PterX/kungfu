<template>
  <div class="code-editor">
    <KfMonacoEditor
      v-if="activeFile !== null && !activeFile.isDir"
      ref="monacoEditor"
      v-model:content="currentContent"
      :options="options"
      @blur="writeCurrentFile"
    />
    <div v-else class="iconfont">
      <code-outlined :style="{ width: '100%', height: '100%' }" />
    </div>
  </div>
</template>
<script setup lang="ts">
import path from 'path';
import fse from 'fs-extra';
import { getCurrentWindow } from '@electron/remote';
import { ref, watch, computed, onMounted } from 'vue';
import { storeToRefs } from 'pinia';
import { CodeOutlined } from '@ant-design/icons-vue';

import KfMonacoEditor from '@kungfu-trader/kungfu-app/src/renderer/components/public/KfMonacoEditor.vue';

import { findTargetFromArray } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { getFileContent } from '@kungfu-trader/kungfu-js-api/utils/fileUtils';
import {
  SpaceTabSettingEnum,
  SpaceSizeSettingEnum,
} from '@kungfu-trader/kungfu-js-api/typings/enums';
import languageJSON from '@kungfu-trader/kungfu-app/src/renderer/assets/monaco/iconFileConfig.json';
import { useCodeStore } from '../store/codeStore';

const monacoEditor = ref<typeof KfMonacoEditor | null>(null);

const { currentFile, fileTree, globalSetting } = storeToRefs(useCodeStore());

const currentContent = ref('');
const activeFile = ref<Code.FileData | null>(null);

const options = computed(() => {
  const language = getFileLanguage(activeFile.value);

  const codeSetting = globalSetting.value.code as Code.ICodeSetting;
  const insertSpaces = codeSetting
    ? codeSetting.tabSpaceType === SpaceTabSettingEnum.SPACES
    : true;
  const tabSize =
    codeSetting?.tabSpaceSize === SpaceSizeSettingEnum.FOURINDENT ? 4 : 2;

  return {
    language,
    insertSpaces,
    tabSize,
  };
});

// 监听文件树变化
watch(fileTree, (newTree, oldTree) => {
  if (
    oldTree['0'] &&
    oldTree['0'].id !== null &&
    oldTree['0'].id !== undefined
  ) {
    const newRootPath = findTargetFromArray<Code.FileData>(
      Object.values(newTree),
      'root',
      true,
    )?.filePath;
    const oldRootPath = findTargetFromArray<Code.FileData>(
      Object.values(oldTree),
      'root',
      true,
    )?.filePath;
    if (newRootPath !== oldRootPath) {
      activeFile.value = null;
    }
  }
});

// 监听当前文件状态
watch(currentFile, async (newFile: Code.FileData) => {
  const filePath: string = newFile.filePath || '';

  if (newFile.isDir) return;
  activeFile.value = newFile as Code.FileData;
  currentContent.value = await getFileContent(filePath);
});

let saveFileCallback: () => void;
function bindEvent() {
  const win = getCurrentWindow();
  if (saveFileCallback) win.removeListener('close', saveFileCallback);

  saveFileCallback = () => {
    writeCurrentFile();
  };
  win.once('close', saveFileCallback);
}

function writeCurrentFile() {
  if (!activeFile.value) return;

  const curPath: string = path.normalize(activeFile.value.filePath);
  fse.outputFileSync(curPath, currentContent.value, 'utf-8');
}

function getFileLanguage(file: Code.FileData | null): string {
  if (file) {
    return file.ext ? languageJSON[file.ext] : 'plaintext';
  }

  return 'plaintext';
}

onMounted(() => {
  bindEvent();
});
</script>
<style lang="less">
.code-editor {
  display: flex;
  justify-content: center;
  align-items: center;
  min-width: 0;

  .margin-view-overlays {
    user-select: none;
  }

  .iconfont {
    font-size: 100px;
    color: #969896;
  }
}
</style>
