<script setup lang="ts">
import { onMounted, reactive, ref, nextTick } from 'vue';
import MainContentVue from './components/MainContent.vue';
import Editor from './components/MonacoEditor.vue';
import FileTree from './components/FileTree.vue';
//公共函数，用来获取跳转过来的URL中携带的参数，用来给窗口设置标题
import { getUrlParams } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/codeUtils';
import {
  removeLoadingMask,
  setHtmlTitle,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';

import { useCodeStore } from './store/codeStore';
const store = useCodeStore();

const urlParmObj = getUrlParams();
setHtmlTitle(urlParmObj.id);

//设置currentNode用来存储当前编辑项的数据信息
const currentNode = reactive<Code.CodeInfo>({
  code_id: '',
  file_path: '',
});

const isEntryFilenameEditable = ref<boolean>(
  urlParmObj.isEntryFilenameEditable
    ? urlParmObj.isEntryFilenameEditable === 'true'
    : true,
);

//filePath用来存入当前点击编辑按钮选中的文件路径
const filePath = ref<string>(decodeURI(urlParmObj.filePath));

let shouldClose = false;

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
    const timer = setTimeout(() => {
      timer && clearTimeout(timer);
      window.close();
    }, 100);
    return false;
  };
}

//当前组件一挂载到页面上以后，将URL中拿到的数据源设置给currentNode。然后传给子组件FileTree
onMounted(() => {
  currentNode.code_id = urlParmObj.id;
  currentNode.file_path = decodeURI(urlParmObj.filePath);
  store.setCurrentCode(currentNode);
  store.getKungfuConfig();
  bindCloseWindowEvent();
  nextTick(() => {
    removeLoadingMask();
  });
});
</script>

<template>
  <div class="code-editor-background">
    <MainContentVue>
      <div class="code-content">
        <FileTree
          :isEntryFilenameEditable="isEntryFilenameEditable"
          :filePath="filePath"
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
