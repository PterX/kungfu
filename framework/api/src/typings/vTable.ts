// Minimal vtable column types for the dealTradingTable hook.
//
// These previously came from the Vue app's @visactor/vtable-backed config
// (@kungfu-trader/kungfu-app/src/renderer/assets/configs/vTable). That created
// an inverted dependency (api -> app) and dragged visactor into the api type
// graph. The quant terminal that consumed these column definitions is being
// retired in the v4 frontend reborn, so the hook's public signature is kept
// here with loose types instead of pulling visactor into the api package.
export type IVTableColumn = Record<string, unknown>;
export type VTableColumnDefine = Record<string, unknown>;
