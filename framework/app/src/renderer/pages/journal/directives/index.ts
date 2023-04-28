export const dragging = {
  beforeMount(el, binding) {
    const handleMouseDown = (event) => {
      binding.value.onMouseDown(event);
    };

    const handleMouseUp = (event) => {
      binding.value.onMouseUp(event);
    };

    el.addEventListener('mousedown', handleMouseDown);
    el.addEventListener('mouseup', handleMouseUp);

    el._handleMouseDown = handleMouseDown;
    el._handleMouseUp = handleMouseUp;
  },
  unmounted(el) {
    el.removeEventListener('mousedown', el._handleMouseDown);
    el.removeEventListener('mouseup', el._handleMouseUp);
  },
};
