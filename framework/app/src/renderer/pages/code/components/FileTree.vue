<template>
  <div class="file-tree">
    <div class="file-tree-content">
      <div class="current-node-name">
        <span class="name">
          {{ $t('editor.current') + $t('folder') }}
        </span>
        <span class="tree-deal-file">
          <span
            class="create"
            :title="$t('editor.open_folder')"
            @click="handleOpenFileLocation()"
          >
            <SelectOutlined class="icon" />
          </span>
          <span
            class="create"
            :title="$t('editor.new_file')"
            v-if="currentCodePath"
            @click="handleAddFile"
          >
            <FileAddFilled class="icon" />
          </span>
          <span
            class="create"
            :title="$t('editor.new_folder')"
            v-if="currentCodePath"
            @click="handleAddFolder"
          >
            <FolderAddFilled class="icon" />
          </span>
        </span>
      </div>
      <div class="file-tree-body" v-if="currentCodePath">
        <div class="scroll-view">
          <div v-for="file in fileTree">
            <FileNode
              v-if="file.root"
              :count="0"
              :fileNode="file"
              :id="file.id"
              type="folder"
              :filePath="filePath"
              :isEntryFilenameEditable="isEntryFilenameEditable"
            ></FileNode>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { watch, ref } from 'vue';
import FileNode from './FileNode.vue';
import { storeToRefs } from 'pinia';
import { useCodeStore } from '../store/codeStore';
import path from 'path';
import { shell } from '@electron/remote';
import {
  FileAddFilled,
  FolderAddFilled,
  SelectOutlined,
} from '@ant-design/icons-vue';
import {
  getTreeByFilePath,
  openFolder,
  buildFileObj,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/codeUtils';
import { findTargetFromArray } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { messagePrompt } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';

const store = useCodeStore();
const props = withDefaults(
  defineProps<{
    currentNode: Code.CodeInfo;
    filePath: string;
    isEntryFilenameEditable?: boolean;
  }>(),
  {
    isEntryFilenameEditable: true,
  },
);

const currentCodePath = ref<string>('');
const currentCodePathName = ref<string>('');
const { currentFile, fileTree } = storeToRefs(useCodeStore());
const { error } = messagePrompt();

//监视currentNode，拿到最新的的数据然后初始化左侧的文件树
watch(props.currentNode as Code.CodeInfo, (newCurrentNode) => {
  currentCodePath.value = path.dirname(newCurrentNode.file_path);
  currentCodePathName.value = path.basename(currentCodePath.value);
  initFileTree(newCurrentNode).then((fileItem) => {
    const entryPath: string = newCurrentNode.file_path;
    const currentFile = findTargetFromArray<Code.FileData>(
      Object.values(fileItem),
      'filePath',
      entryPath,
    );
    if (currentFile) {
      store.setEntryFile(currentFile);
      store.setCurrentFile(currentFile);
    }
  });
});

function handleOpenFileLocation() {
  const filePath = fileTree.value[0].filePath;
  if (!filePath) {
    error();
    return;
  }
  return shell.showItemInFolder(filePath);
}

//加文件夹
function handleAddFolder() {
  const id: number = currentFile.value.id;
  const rootId: number = +store.getRootFileId;
  const target = fileTree.value[id] || fileTree.value[rootId];
  if (target.isDir) {
    openFolder(target, fileTree.value, true).then(() => {
      store.addFileFolderPending({ id: target.id, type: 'folder' });
    });
  } else {
    if (target.parentId !== undefined && !isNaN(+target.parentId)) {
      store.addFileFolderPending({
        id: target.parentId,
        type: 'folder',
      });
    } else {
      throw new Error();
    }
  }
}

//加文件
function handleAddFile() {
  const id = currentFile.value.id;
  const rootId: number = +store.getRootFileId;
  const target = fileTree.value[id] || fileTree.value[rootId];
  if (target.isDir) {
    openFolder(target, fileTree.value, true).then(() => {
      store.addFileFolderPending({ id: target.id, type: '' });
    });
  } else {
    if (target.parentId !== undefined && !isNaN(+target.parentId)) {
      store.addFileFolderPending({ id: target.parentId, type: '' });
    } else {
      throw new Error();
    }
  }
}

async function initFileTree(currentNode) {
  if (!currentNode.code_id || !currentNode.file_path) return;
  const rootId = window.fileId++;
  const rootFile: Code.FileData = buildFileObj({
    id: rootId,
    parentId: 0,
    isDir: true,
    name: currentCodePathName.value,
    ext: '',
    filePath: currentCodePath.value,
    children: { file: [], folder: [] },
    stats: {},
    root: true,
    open: true,
    fileId: 1,
  });
  //获取第一级文件树
  let ids, rootFileTree;
  try {
    const fileTreeData = await getTreeByFilePath(rootFile, rootFileTree);
    ids = fileTreeData.ids;
    rootFileTree = fileTreeData.fileTree;
  } catch (err) {
    if ((<Error>err).message) {
      error((<Error>err).message);
    }
  }

  // 处理根
  rootFile['children'] = ids;
  rootFileTree[rootId] = rootFile;
  // padding
  rootFileTree = bindFunctionalNode(rootFileTree);
  // currrentFileTree.value = rootFileTree;
  store.setFileTree(rootFileTree);
  store.setCurrentFile(rootFile);

  return rootFileTree;
}

//增加 以pending 为key 的node，为addfile做准备
function bindFunctionalNode(curFileTree) {
  curFileTree['pending'] = {
    parentId: '',
  };
  return curFileTree;
}
</script>

<style lang="less">
.file-tree {
  width: 300px;
  padding-top: 8px;
  text-align: center;
  display: flex;
  flex-direction: column;
  background: @component-background;
  user-select: none;

  .open-editor-folder {
    width: 90%;
    height: 36px;
    margin: auto;
  }

  .current-node-name {
    font-size: 14px;
    font-weight: bolder;
    margin-top: 8px;
    margin-bottom: 8px;
    height: 30px;
    line-height: 30px;
    padding: 0px 8px;
    border-top: none;
    border-bottom: 2px solid @component-background;
    text-align: left;
    box-sizing: border-box;
    color: @text-color;
    .name {
      span {
        vertical-align: bottom;
        display: inline-block;
        max-width: 160px;
        text-overflow: ellipsis;
        white-space: nowrap;
        overflow: hidden;
      }
    }
    .tree-deal-file {
      display: none;
    }
    .folder-oper {
      cursor: pointer;
      display: none;
      padding: 0 6px;
      i {
        font-size: 15px;
      }
    }
  }
  .folder-name {
    height: 30px;
    line-height: 30px;
    color: @text-color;
    text-align: left;
    display: flex;
    flex-direction: row;
    padding-left: 5px;
    .name {
      max-width: 100px;
      font-size: 14px;
      padding-right: 20px;
      color: @text-color;
    }
    .title {
      color: @input-bg;
    }
  }
  .file-tree-content {
    height: calc(100% - 40px);
  }
  .file-tree-body {
    height: calc(100% - 38px);
    overflow: auto;
    padding-left: 5px;
    user-select: none;
    &::-webkit-scrollbar {
      display: none;
    }
  }
  &:hover {
    .tree-deal-file {
      display: block;
      float: right;
      margin-right: 10px;
      .create {
        margin: 0 2px;
        cursor: pointer;
        .icon {
          color: @text-color;
          &:hover {
            color: @icon-color-hover;
          }
        }
      }
    }
  }
}
</style>
