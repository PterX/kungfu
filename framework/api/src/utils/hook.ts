import { watch } from 'vue';

export function useDynamicStyle(elementRef, className, styleObject) {
  const applyStyle = () => {
    let element = elementRef.value;

    // 如果是一个 Vue 组件实例，则获取其根 DOM 元素
    if (element?.$el) {
      element = element.$el;
    }

    if (!element) return;

    const targetElements = element.getElementsByClassName(className);

    for (const targetElement of targetElements) {
      Object.assign(targetElement.style, styleObject);
    }
  };

  watch(() => [elementRef.value, className, styleObject], applyStyle, {
    immediate: true,
  });

  return { applyStyle };
}
