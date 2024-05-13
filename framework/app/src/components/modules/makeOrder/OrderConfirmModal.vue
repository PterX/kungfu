<script setup lang="ts">
import {
  messagePrompt,
  useModalVisible,
} from '@kungfu-trader/kungfu-app/src/renderer/assets/methods/uiUtils';
import { dealKfDecimalPrecision } from '@kungfu-trader/kungfu-js-api/utils/commonUtils';
import { InstrumentTypeEnum } from '@kungfu-trader/kungfu-js-api/typings/enums';
import { ref, toRefs, computed, getCurrentInstance, onMounted } from 'vue';
import VueI18n from '@kungfu-trader/kungfu-js-api/language';
import {
  isShotable,
  isCryptoInstrument,
  getPrecisionByInstrumentType,
} from '@kungfu-trader/kungfu-js-api/utils/tradingUtils';
const { t } = VueI18n.global;

const { error } = messagePrompt();
const app = getCurrentInstance();

const props = withDefaults(
  defineProps<{
    visible: boolean;
    curOrderVolume: number;
    curOrderType: InstrumentTypeEnum;
    volumePrecision?: number;
    volumeStep?: number;
  }>(),
  {
    visible: false,
    volumePrecision: 0,
    volumeStep: 1,
  },
);

defineEmits<{
  (e: 'confirm', volumeList: number[]): void;
  (e: 'update:visible', visible: boolean): void;
  (e: 'close'): void;
}>();

const everyVolumeInput = ref<HTMLElement | null>(null);

onMounted(() => {
  if (everyVolumeInput.value) {
    everyVolumeInput.value.focus();
  }
});

function isDivisible(dividend: number, divisor: number) {
  if (divisor === 0) {
    return false;
  }

  const result = dividend / divisor;

  const epsilon = 1e-13;

  const difference = Math.abs(result - Math.round(result));
  return difference < epsilon;
}

const orderNumber = computed(() => {
  return volume.value
    ? Math.floor(
        dealKfDecimalPrecision(curOrderVolume.value / volume.value, precision),
      )
    : 0;
});

const apartOrderNumber = computed(() => {
  return volume.value
    ? isDivisible(curOrderVolume.value, volume.value)
      ? orderNumber.value
      : orderNumber.value + 1
    : 0;
});

const { modalVisible, closeModal } = useModalVisible(props.visible);
const { curOrderType } = props;
const { curOrderVolume, volumePrecision, volumeStep } = toRefs(props);

const defaultVolume = isCryptoInstrument(curOrderType)
  ? volumeStep.value
  : isShotable(curOrderType)
  ? 1
  : 100;
const precision = getPrecisionByInstrumentType(curOrderType);
const volume = ref<number>(defaultVolume);

function handleConfirm() {
  if (volume.value === null) {
    error(t('tradingConfig.no_empty'));
    return;
  }

  const volumeList: number[] = new Array(+orderNumber.value).fill(volume.value);

  if (!isDivisible(curOrderVolume.value, volume.value)) {
    const remainder: number = dealKfDecimalPrecision(
      curOrderVolume.value % volume.value,
      precision,
    ); // 剩余数量
    volumeList.push(remainder);
  }

  app && app.emit('confirm', volumeList);
  closeModal();
}
</script>

<template>
  <a-modal
    class="kf-order-confirm-modal"
    :width="420"
    v-model:visible="modalVisible"
    :title="$t('tradingConfig.apart_order')"
    :destroyOnClose="true"
    @cancel="closeModal"
    @ok="handleConfirm"
  >
    <a-row class="apart-input">
      <a-col>
        <a-statistic
          :value="curOrderVolume"
          :title="$t('tradingConfig.total_order_amount')"
        />
        <a-input-group compact style="margin-top: 10px" class="input-content">
          <span>{{ $t('tradingConfig.every_volume') }}:</span>
          <a-input-number
            ref="everyVolumeInput"
            class="input-number"
            :precision="volumePrecision"
            :step="volumeStep"
            v-model:value="volume"
            :max="curOrderVolume"
            :min="volumeStep"
          ></a-input-number>
        </a-input-group>
      </a-col>
      <a-col class="apart-result">
        <a-statistic
          class="apart-result-statistic"
          :value="apartOrderNumber || '--'"
          :valueStyle="{ fontSize: '35px' }"
          :title="$t('tradingConfig.make_order_number')"
        />
      </a-col>
    </a-row>
  </a-modal>
</template>
<style lang="less">
.kf-order-confirm-modal {
  .apart-result {
    text-align: center;
    margin: auto;
  }
  .input-number {
    margin-left: 10px;
  }
  .input-content {
    span {
      line-height: 32px;
    }
  }
}
</style>
