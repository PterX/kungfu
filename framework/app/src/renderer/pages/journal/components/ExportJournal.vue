<template>
  <a-button @click="handleOpenExportFormModal">
    {{ t('journalConfig.export') }}
  </a-button>

  <a-modal
    v-model:visible="exportFormModalVisible"
    :width="520"
    class="kf-set-by-config-modal"
    :title="exportModalConfig.title"
    :destroy-on-close="true"
    :cancel-text="$t('cancel')"
    :ok-text="$t('confirm')"
    @ok="handleConfirmExport"
  >
    <a-form
      ref="formRef"
      class="kf-config-form"
      :model="exportFormState"
      :colon="false"
      :scroll-to-first-error="true"
    >
      <a-form-item
        :key="EXPORT_KEY"
        :rules="rules"
        :label="exportModalConfig.name"
        :required="true"
      >
        <div class="kf-form-item__warp file">
          <a-button size="small" @click="handleSelectFile">
            <template #icon><DashOutlined /></template>
          </a-button>
          <div
            class="file-path"
            :title="(exportFormState[EXPORT_KEY] || '').toString()"
          >
            <span class="name">{{ exportFormState[EXPORT_KEY] }}</span>
          </div>
        </div>
      </a-form-item>
    </a-form>
  </a-modal>
</template>

<script setup lang="ts">
import fse from 'fs-extra';
import path from 'path';
import { dialog } from '@electron/remote';
import { reactive, ref } from 'vue';
import { DashOutlined } from '@ant-design/icons-vue';
import { message } from 'ant-design-vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import { writeCsvByStream } from '../utils';

const { t } = VueI18n.global;

const props = withDefaults(
  defineProps<{
    exportData: KungfuApi.FrameResolved[];
    fileName: string;
  }>(),
  {
    exportData: () => [],
    fileName: '',
  },
);

const EXPORT_KEY = 'export_file_path';
const formRef = ref();
const exportFormState = reactive({
  [EXPORT_KEY]: '',
});
const exportFormModalVisible = ref(false);

const exportModalConfig = {
  title: t('journalConfig.export'),
  name: t('journalConfig.export_file_path'),
};

const rules = {
  required: true,
  type: 'string',
  validator: (_rule, value) => {
    console.log(value);
    return fse
      .stat(value as string)
      .then((stats) => {
        console.log(stats);
        if (stats.isDirectory()) return Promise.resolve();

        return Promise.reject(new Error(t('journalConfig.need_directroy')));
      })
      .catch((err) => {
        return Promise.reject(err);
      });
  },
  trigger: 'change',
};

const handleOpenExportFormModal = () => {
  exportFormModalVisible.value = true;
};

function handleSelectFile(): void {
  dialog
    .showOpenDialog({
      properties: ['openDirectory'],
    })
    .then((res) => {
      const { filePaths } = res;
      if (filePaths.length) {
        exportFormState[EXPORT_KEY] = filePaths[0];
        formRef.value.validateFields([EXPORT_KEY]); //手动进行再次验证, 因数据放在span中, 改变数据后无法触发验证
      }
    });
}

const handleConfirmExport = () => {
  if (props.exportData) {
    formRef.value?.validate().then(() => {
      if (!exportFormState[EXPORT_KEY]) {
        message.error('导出目录不能为空');
        return;
      }
      const headers = Object.keys(props.exportData).sort((a, b) =>
        a.localeCompare(b),
      );
      writeCsvByStream<KungfuApi.FrameResolved>(
        path.join(exportFormState[EXPORT_KEY], props.fileName + '.csv'),
        props.exportData,
        headers,
      )
        .then((res) => {
          if (res) {
            message.success(t('journalConfig.export_success'));
            exportFormModalVisible.value = false;
          }
        })
        .catch((err) => {
          message.error(err);
        });
    });
  }
};
</script>

<style lang="less">
.kf-form-item__warp {
  &.file {
    padding-bottom: 4px;

    div.file-path {
      word-break: break-word;
      margin-top: 8px;
      box-sizing: border-box;

      .name {
        padding-right: 16px;
        box-sizing: border-box;
      }
    }

    button {
      width: 40px;
    }
  }
}
</style>
