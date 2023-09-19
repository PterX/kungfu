<template>
  <a-popover
    v-model:visible="popoverVisible"
    :overlay-style="{ width: '420px' }"
    placement="topRight"
    trigger="click"
  >
    <template #title>
      <div class="kf-update-controller-title__wrap">
        <span>{{ $t('autoUpdater.update') }}</span>
        <CloseOutlined @click="popoverVisible = false" />
      </div>
    </template>
    <template #content>
      <template v-if="hasNewVersion">
        <div v-if="downloadStarted" style="padding-right: 16px">
          <a-progress
            :percent="process"
            :status="progressStatus"
            stroke-color="#FAAD14"
          ></a-progress>
          <div v-if="process === 100">
            <span>{{ $t('autoUpdater.downloaded') }}</span>
            <a-button
              style="margin-left: 8px"
              type="link"
              @click="handleQuitAndInstall"
            >
              {{ $t('autoUpdater.to_install') }}
            </a-button>
          </div>
          <div v-else-if="errorMessage">
            <span class="color-red">{{ errorMessage }}</span>
            <a-button type="link" @click="handleToStartDownload">
              {{ $t('autoUpdater.retry_download') }}
            </a-button>
          </div>
        </div>
        <div v-else-if="hasSkiped">
          <span>
            {{ $t('autoUpdater.current_version') + ': ' + oldVersion }}
          </span>
          <span style="margin-left: 8px">
            {{
              $t('autoUpdater.new_version') +
              ': ' +
              newVersion +
              `(${$t('autoUpdater.has_skiped')})`
            }}
          </span>
          <a-button
            type="link"
            size="small"
            :loading="checkingUpdate"
            @click="handleToRetryCheckUpdate"
          >
            {{ $t('autoUpdater.retry_check') }}
          </a-button>
        </div>
        <div v-else>
          <span>
            {{ $t('autoUpdater.new_version') + ': ' + newVersion }}
          </span>
          <a-button type="link" @click="handleToStartDownload">
            {{ $t('autoUpdater.start_download') }}
          </a-button>
          <a-button
            type="link"
            style="padding-left: 0px"
            @click="skipVersion(newVersion)"
          >
            {{ $t('autoUpdater.skip_version') }}
          </a-button>
        </div>
      </template>
      <template v-else>
        <span>
          {{
            $t('autoUpdater.current_version') +
            ': ' +
            currentVersion +
            ' ( ' +
            $t('autoUpdater.already_latest_version') +
            ' )'
          }}
        </span>
        <a-button
          type="link"
          size="small"
          :loading="checkingUpdate"
          @click="handleToRetryCheckUpdate"
        >
          {{ $t('autoUpdater.retry_check') }}
        </a-button>
      </template>
    </template>
    <div class="kf-update-controller-entry__wrap">
      <interaction-outlined />
      <span style="margin-left: 4px">
        {{ $t('autoUpdater.update') }}
      </span>
    </div>
  </a-popover>
</template>

<script lang="ts" setup>
import { InteractionOutlined, CloseOutlined } from '@ant-design/icons-vue';
import { useUpdateVersion } from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/actionsUtils';

const {
  popoverVisible,
  newVersion,
  oldVersion,
  hasSkiped,
  currentVersion,
  checkingUpdate,
  hasNewVersion,
  downloadStarted,
  process,
  progressStatus,
  errorMessage,
  handleToRetryCheckUpdate,
  handleToStartDownload,
  skipVersion,
  handleQuitAndInstall,
} = useUpdateVersion();
</script>

<style lang="less">
.kf-update-controller-title__wrap {
  width: 100%;
  display: flex;
  justify-content: space-between;
  align-items: center;
}
</style>
