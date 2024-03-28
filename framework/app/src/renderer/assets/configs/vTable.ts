import * as VTable from '@visactor/vtable';
import * as VTable_editors from '@visactor/vtable-editors';
import caret_normal from '../svg/table-icons/caret-normal.svg';
import caret_upward from '../svg/table-icons/caret-upward.svg';
import caret_downward from '../svg/table-icons/caret-downward.svg';
import {
  type ITextGraphicAttribute,
  IImageGraphicAttribute,
} from '@visactor/vrender-core';
export type ITextOption = {
  type: string;
  marginTop?: number;
  marginRight?: number;
  marginBottom?: number;
  marginLeft?: number;
  key: string;
  dealValue?: (value) => string | string;
} & ITextGraphicAttribute;
export type IImageOptions = {
  type: string;
  src?: string;
  shape?: 'circle' | 'square';
  marginTop?: number;
  marginRight?: number;
  marginBottom?: number;
  marginLeft?: number;
  key: string;
  dealValue?: (value) => string | string;
} & IImageGraphicAttribute;

export type ICustomActionOption = ITextOption & IImageOptions;
export interface InputEditorConfig {
  max?: number;
  min?: number;
}

// class CustomInputEditor implements VTable_editors.IEditor {
//   style: Record<string, string> = {};
//   editorType = 'Input';
//   editorConfig;
//   container;
//   declare element: HTMLInputElement;
//   referencePosition: {
//     rect: VTable_editors.RectProps;
//     placement?: VTable_editors.Placement;
//   } | null = null;
//   activeCell: HTMLElement | null = null; // Track the currently active cell

//   constructor(
//     editStyle: Record<string, string> = {},
//     editorConfig?: InputEditorConfig,
//   ) {
//     if (editorConfig) this.editorConfig = editorConfig;
//     this.style = editStyle;
//   }

//   createElement() {
//     const input = document.createElement('input');
//     input.setAttribute('type', 'text');
//     input.style.position = 'absolute';
//     input.style.padding = '4px';
//     input.style.width = '100%';
//     input.style.color = 'black';
//     input.style.boxSizing = 'border-box';
//     Object.keys(this.style).forEach((key) => {
//       input.style[key] = this.style[key];
//     });

//     this.element = input;

//     this.container.appendChild(input);
//   }

//   setValue(value: string) {
//     this.element.value = typeof value !== 'undefined' ? value : '';
//   }

//   getValue() {
//     return this.element.value;
//   }

//   beginEditing(
//     container: HTMLElement,
//     referencePosition: {
//       rect: VTable_editors.RectProps;
//       placement?: VTable_editors.Placement;
//     },
//     value?: string,
//   ) {
//     window.addEventListener('resize', this.handleResize);
//     this.container = container;
//     this.referencePosition = referencePosition;

//     this.createElement();
//     if (value) {
//       this.setValue(value);
//     }
//     if (referencePosition?.rect) {
//       this.adjustPosition(referencePosition.rect);
//     }
//     this.element.focus();
//     // do nothing
//   }

//   handleResize = () => {
//     if (this.container) {
//       // if (this.referencePosition?.rect) {
//       //   this.adjustPosition(this.referencePosition.rect);
//       // }
//       this.exit();
//       // const rect = this.activeCell.getBoundingClientRect();
//       // this.adjustPosition(rect);
//     }
//   };

//   adjustPosition(rect: VTable_editors.RectProps) {
//     this.element.style.top = rect.top + 'px';
//     this.element.style.left = rect.left + 'px';
//     this.element.style.width = rect.width + 'px';
//     this.element.style.height = rect.height + 'px';
//   }

//   endEditing() {
//     // do nothing
//   }

//   onStart() {
//     // do nothing
//   }

//   onEnd() {
//     // do nothing
//   }

//   exit() {
//     // do nothing
//     if (this.container) {
//       window.removeEventListener('resize', this.handleResize);
//       // Check if this.element is a child of this.container
//       if (this.container.contains(this.element)) {
//         // Remove the active cell and its associated editor
//         this.container.removeChild(this.element);
//       }
//     }
//   }

//   targetIsOnEditor(target: HTMLElement) {
//     if (target === this.element) {
//       return true;
//     }
//     return false;
//   }
// }

const registerVTableIconsAndEditors = () => {
  VTable.register.icon('sort_upward', {
    type: 'svg',
    svg: caret_upward,
    width: 18,
    height: 18,
    name: 'sort_upward',
    positionType: VTable.TYPES.IconPosition.right,
    marginRight: -10,
    funcType: VTable.TYPES.IconFuncTypeEnum.sort,
    hover: {
      width: 18,
      height: 18,
      bgColor: '#444',
      //颜色深一些

      //bgColor: 'rgba(101, 117, 168, 0.1)',
    },
    cursor: 'pointer',
  });
  VTable.register.icon('sort_normal', {
    type: 'svg',
    svg: caret_normal,
    width: 18,
    height: 18,
    name: 'sort_normal',
    positionType: VTable.TYPES.IconPosition.right,
    marginRight: -10,
    funcType: VTable.TYPES.IconFuncTypeEnum.sort,
    hover: {
      width: 18,
      height: 18,
      bgColor: '#444',
    },
    cursor: 'pointer',
  });

  VTable.register.icon('sort_downward', {
    type: 'svg',
    svg: caret_downward,
    width: 18,
    height: 18,
    name: 'sort_downward',
    positionType: VTable.TYPES.IconPosition.right,
    marginRight: -10,
    funcType: VTable.TYPES.IconFuncTypeEnum.sort,
    hover: {
      width: 18,
      height: 18,
      bgColor: '#444',
    },
    cursor: 'pointer',
  });

  // const input_editor = new CustomInputEditor({
  //   color: '#ffffffd9',
  //   background: '#141414',
  // });
  // VTable.register.editor('input-editor', input_editor);
};

registerVTableIconsAndEditors();

export { VTable, VTable_editors };
