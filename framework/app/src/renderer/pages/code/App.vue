<script setup lang="ts">
//vue相关函数、组件等
import { onMounted, reactive, ref } from 'vue';
import MainContentVue from './components/MainContent.vue';
import Editor from './components/MonacoEditor.vue';
import FileTree from './components/FileTree.vue';
//公共函数，用来获取跳转过来的URL中携带的参数，用来给窗口设置标题
import { getUrlParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/codeUtils';
import { setHtmlTitle } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';

// import { useCodeStore } from './store/codeStore';
// import { ipcEmitDataByName } from '../../../renderer/ipcMsg/emitter';
// import VueI18n from '@kungfu-trader/kungfu-js-api/language';
// const { t } = VueI18n.global;
// const { error } = messagePrompt();
// const store = useCodeStore();

//获取到当前的URL中的参数，然后通过processId字段给窗口设置标题
const urlParmObj = getUrlParams();
const processId = urlParmObj.processId;
setHtmlTitle(processId);

//设置currentNode用来存储当前编辑项的数据信息
const currentNode = reactive<Code.Icodeinfo>({
  code_id: '',
  file_path: '',
  add_time: 0,
});
// fileTreeType用来存入code编辑器当前需要展示的类型，是策略还是算子等
const fileTreeType = ref<string>('');
//filePath用来存入当前点击编辑按钮选中的文件路径
const filePath = ref<string>('');
//将URL中带过来的路径存入filePath
filePath.value = urlParmObj.file_path;

// 处理Object格式strageList
// function handleStrategyList(strategyList): void {
//   const value: Code.Icodeinfo = strategyList[0];

//   currentNode.code_id = value.code_id;
//   currentNode.file_path = value.file_path;
//   currentNode.add_time = value.add_time;
//   store.setCurrentStrategy(currentNode);
// }

// function handleUpdateStrategy(strategyPath) {
// if (!currentNode.code_id) {
//   error(t('code_id不存在!'));
//   return;
// }
// currentNode.file_path = strategyPath;
// updateStrategy(currentNode.code_id, strategyPath);
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

//绑定窗口关闭事件
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

//当前组件一挂载到页面上以后，将URL中拿到的数据源设置给currentNode。然后传给子组件FileTree
onMounted(() => {
  currentNode.code_id = urlParmObj.processId;
  currentNode.file_path = urlParmObj.file_path;
  currentNode.add_time = new Date().getTime();
  const categoryStr = urlParmObj.processId.split('_')[0];
  fileTreeType.value = categoryStr;
  bindCloseWindowEvent();
});
</script>

<template>
  <div class="code-editor-background">
    <MainContentVue>
      <div class="code-content">
        <FileTree
          :filePath="filePath"
          :fileTreeType="fileTreeType"
          :currentNode="currentNode"
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
