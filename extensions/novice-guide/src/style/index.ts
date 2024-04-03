import { Decl } from '../utils';

export const driverStyleRules: Decl[] = [
  ['.driver-active', ['pointer-events', 'all', true]],
  [
    '.driver-popover-arrow-side-left.driver-popover-arrow',
    ['border-left-color', '#1f1f1f', true],
  ],
  [
    '.driver-popover-arrow-side-right.driver-popover-arrow',
    ['border-right-color', '#1f1f1f', true],
  ],
  [
    '.driver-popover-arrow-side-top.driver-popover-arrow',
    ['border-top-color', '#1f1f1f', true],
  ],
  [
    '.driver-popover-arrow-side-bottom.driver-popover-arrow',
    ['border-bottom-color', '#1f1f1f', true],
  ],
  [
    '.driver-popover-title',
    ['font-weight', '200', true],
    ['font-size', '17px', true],
  ],
  [
    '.driver-popover-description',
    ['white-space', 'break-spaces', true],
    ['font-weight', '200', true],
    ['margin-top', '12px', true],
  ],
  ['.driver-popover-footer', ['margin-top', '24px', true]],
  [
    '.driver-popover-close-btn',
    ['top', '8px', true],
    ['right', '4px', true],
    ['font-size', '16px', true],
    ['color', 'rgba(255, 255, 255, 0.85)', true],
  ],
  [
    '.kf-driver',
    ['padding', '12px 20px 20px', true],
    ['background', '#1f1f1f', true],
    ['color', 'rgba(255, 255, 255, 0.85)', true],
  ],
  [
    '.kf-driver .driver-popover-navigation-btns button',
    ['color', 'rgba(255, 255, 255, 0.85)', true],
    ['background', 'transparent', true],
    ['border', '1px solid #434343', true],
    ['padding', '6px 10px', true],
    ['text-shadow', 'none', true],
  ],
  [
    '.kf-driver .driver-popover-navigation-btns button:hover',
    ['background', 'transparent', true],
    ['border', '1px solid #aa7714', true],
    ['color', '#aa7714', true],
  ],
  ['.kf-driver-modal', ['transform', 'translateY(-16vh)', true]],
  ['.ant-btn[disabled]', ['pointer-events', 'none', true]],
];
