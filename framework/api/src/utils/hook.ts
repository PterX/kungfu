import { watch } from 'vue';

if (!globalThis.globalBoardStylesMap) {
  globalThis.globalBoardStylesMap = new Map();

  document.addEventListener('keydown', function (e) {
    if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.code.startsWith('Digit')) {
      const boardNumber = parseInt(e.code.replace('Digit', ''), 10);
      const board = globalThis.globalBoardStylesMap.get(boardNumber);

      if (board) {
        board.focus();
        board.classList.add('kf-highlight-outline');
        setTimeout(() => {
          board.classList.remove('kf-highlight-outline');
        }, 300);
        const methods = globalThis.globalBoardStylesMap.get(board);
        if (!methods) return;
        for (const method of methods) {
          method();
        }
      }
    }

    if (e.code === 'Tab') {
      for (const board of globalThis.globalBoardStylesMap.keys()) {
        if (board && board === document.activeElement) {
          const methods = globalThis.globalBoardStylesMap.get(board);
          for (const method of methods) {
            method();
          }
        }
      }
    }
  });
}

export function useDynamicStyle(
  boardRef,
  elementRef,
  uniqueClassName,
  styleString,
  immediate = true,
  shortcutNumber,
) {
  let element = elementRef.value;
  let board = boardRef.value;

  if (board?.$el) {
    board = board.$el;
  }

  if (element?.$el) {
    element = element.$el;
  }

  const styleEl = document.createElement('style');
  document.head.appendChild(styleEl);
  styleEl.textContent = `.${uniqueClassName} ${styleString}`;

  const addStyle = () => {
    if (!element) return;
    element.classList.add(uniqueClassName);
  };

  const removeStyle = () => {
    if (!element) return;
    element.classList.remove(uniqueClassName);
  };

  const methods = globalThis.globalBoardStylesMap.get(board) || [];
  methods.push(addStyle);
  globalThis.globalBoardStylesMap.set(shortcutNumber, board);
  globalThis.globalBoardStylesMap.set(board, methods);

  board.addEventListener('focus', () => board.focus());
  function isChildOf(parent, child) {
    let node = child.parentNode;
    while (node !== null) {
      if (node === parent) {
        return true;
      }
      node = node.parentNode;
    }
    return false;
  }

  board.addEventListener('focusout', () => {
    setTimeout(() => {
      if (!isChildOf(board, document.activeElement)) {
        removeStyle();
      }
    });
  });

  watch(
    () => [elementRef.value, styleString],
    () => {
      element = elementRef.value;
      if (element?.$el) {
        element = element.$el;
      }
    },
    { immediate },
  );

  const cleanup = () => {
    const methods = globalThis.globalBoardStylesMap.get(board);
    const index = methods.indexOf(addStyle);
    if (index > -1) {
      methods.splice(index, 1);
    }
    if (methods.length === 0) {
      globalThis.globalBoardStylesMap.delete(board);
    }
  };

  function loopFocusWithinBoard(board) {
    const focusableElements = board.querySelectorAll(
      'a[href], button, textarea, input[type="text"], input[type="radio"], input[type="checkbox"], select',
    );
    const firstFocusableElement = focusableElements[0];
    const lastFocusableElement =
      focusableElements[focusableElements.length - 1];
    board.addEventListener('keydown', function (e) {
      if (
        e.key === 'Tab' &&
        !e.shiftKey &&
        document.activeElement === lastFocusableElement
      ) {
        e.preventDefault();
        firstFocusableElement.focus();
      }
    });
  }

  loopFocusWithinBoard(board);

  return { addStyle, removeStyle, cleanup };
}
