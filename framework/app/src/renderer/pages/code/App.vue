<script setup lang="ts">
import { onMounted, reactive, watchEffect, ref } from 'vue';
import { getUrlParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/codeUtils';
import {
  // messagePrompt,
  // removeLoadingMask,
  setHtmlTitle,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import Editor from './components/MonacoEditor.vue';
import FileTree from './components/FileTree.vue';
import { useCodeStore } from './store/codeStore';
// import { ipcEmitDataByName } from '../../../renderer/ipcMsg/emitter';
import MainContentVue from './components/MainContent.vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
const { t } = VueI18n.global;

// const { error } = messagePrompt();
const store = useCodeStore();

const urlParmObj: Record<string, string> = getUrlParams();
const processId = urlParmObj['processId'];
const dirPathList = urlParmObj['file_path'].split('/');
dirPathList.pop();
const dirPath = dirPathList.join('/');
console.log(dirPath, 'urlparm99999======');

setHtmlTitle(processId);

const currentCode = reactive<Code.Icodeinfo>({
  code_id: '',
  file_path: '',
  add_time: 0,
});
const fileTreeType = ref<string>('');
const filePath = ref<string>('');

filePath.value = urlParmObj['file_path'];

// const strategyName = processId.split('_')[1];
// const curnStrategyIndex: {
//   value: number;
// } = {
//   value: 0,
// };
// 处理JSON格式strangeList
// function handleStrategyJsonList(strategyList): void {
//   getCurrentStrategy(strategyList);
//   const value: Code.Icodeinfo = JSON.parse(
//     strategyList[curnStrategyIndex.value].value,
//   );
//   currentCode.code_id = value.code_id;
//   currentCode.file_path = value.file_path;
//   currentCode.add_time = value.add_time;
//   store.setCurrentStrategy(currentCode);
// }

// function getCurrentStrategy(strategyList) {
//   strategyList.forEach((item, index) => {
//     if (item.name === strategyName) {
//       curnStrategyIndex.value = index;
//     }
//   });
// }

// 处理Object格式strageList
// function handleStrategyList(strategyList): void {
//   const value: Code.Icodeinfo = strategyList[0];

//   currentCode.code_id = value.code_id;
//   currentCode.file_path = value.file_path;
//   currentCode.add_time = value.add_time;
//   store.setCurrentStrategy(currentCode);
// }

// function handleUpdateStrategy(strategyPath) {
//   if (!currentCode.code_id) {
//     error(t('策略id不存在!'));
//     return;
//   }
//   updateStrategy(currentCode.code_id, strategyPath);
// }

// async function updateStrategy(strategyId: string, strategyPath: string) {
//   await getStrategyById(strategyId);
// }

let shouldClose = false;

// async function getStrategyById(strategyId: string) {
//   const { data } = (await ipcEmitDataByName('strategyById', {
//     strategyId,
//   })) as Record<string, Array<Code.Icodeinfo>>;
//   handleStrategyList(data);
// }

function bindCloseWindowEvent() {
  shouldClose = false;

  window.onbeforeunload = (e) => {
    e.preventDefault(e);
    if (shouldClose) return undefined;
    const $textareaList = document.querySelectorAll('textarea');
    $textareaList.forEach(($textarea) => {
      $textarea && $textarea.blur();
    });
    shouldClose = true;
    setTimeout(() => {
      window.close();
    }, 100);
    return false;
  };
}

onMounted(() => {
  // ipcEmitDataByName('strategyList').then(({ data }) => {
  //   console.log(data, 'data7777777==9999');
  //   store.setStrategyList(data);
  //   nextTick().then(() => {
  //     handleStrategyJsonList(store.strategyList);
  //     removeLoadingMask();
  //   });
  // });

  currentCode.code_id = urlParmObj.processId;
  currentCode.file_path = urlParmObj.file_path;
  currentCode.add_time = new Date().getTime();
  store.setCurrentStrategy(currentCode);
  store.getKungfuConfig();
  bindCloseWindowEvent();
});

watchEffect(() => {
  const categoryStr = processId.split('_')[0];
  fileTreeType.value = categoryStr;
});
</script>

<template>
  <div class="code-editor-background">
    <MainContentVue>
      <div class="code-content">
        <FileTree
          :filePath="filePath"
          :fileTreeType="fileTreeType"
          :currentCode="currentCode"
        ></FileTree>
        <Editor class="editor" ref="code-editor"></Editor>
      </div>
    </MainContentVue>
  </div>
</template>

<style lang="less">
.code-editor-background {
  width: 100%;
  .code-content {
    width: 100%;
    display: flex;
    height: calc(100vh - 32px);
    background-color: #1d1f21;
    .editor {
      text-align: left;
      flex: 1;
    }
  }
}
</style>
