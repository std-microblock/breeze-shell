import * as shell from "mshell"

export const languages = {
    'zh-CN': {
    },
    'en-US': {
        '管理 Breeze Shell': 'Manage Breeze Shell',
        '插件市场 / 更新本体': 'Plugin Market / Update Shell',
        '加载中...': 'Loading...',
        '更新中...': 'Updating...',
        '新版本已下载，将于下次重启资源管理器生效': 'New version downloaded, will take effect next time the file manager is restarted',
        '更新失败: ': 'Update failed: ',
        '插件安装成功: ': 'Plugin installed: ',
        '当前源: ': 'Current source: ',
        '删除': 'Delete',
        '版本: ': 'Version: ',
        '作者: ': 'Author: '
    }
}

export const ICON_EMPTY = new shell.value_reset()
export const ICON_CHECKED = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 12 12"><path fill="currentColor" d="M9.765 3.205a.75.75 0 0 1 .03 1.06l-4.25 4.5a.75.75 0 0 1-1.075.015L2.22 6.53a.75.75 0 0 1 1.06-1.06l1.705 1.704l3.72-3.939a.75.75 0 0 1 1.06-.03"/></svg>`
export const ICON_CHANGE = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="currentColor" d="m17.66 9.53l-7.07 7.07l-4.24-4.24l1.41-1.41l2.83 2.83l5.66-5.66zM4 12c0-2.33 1.02-4.42 2.62-5.88L9 8.5v-6H3l2.2 2.2C3.24 6.52 2 9.11 2 12c0 5.19 3.95 9.45 9 9.95v-2.02c-3.94-.49-7-3.86-7-7.93m18 0c0-5.19-3.95-9.45-9-9.95v2.02c3.94.49 7 3.86 7 7.93c0 2.33-1.02 4.42-2.62 5.88L15 15.5v6h6l-2.2-2.2c1.96-1.82 3.2-4.41 3.2-7.3"/></svg>`
export const ICON_REPAIR = `<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 24 24"><path fill="currentColor" d="M15.73 3H8.27L3 8.27v7.46L8.27 21h7.46L21 15.73V8.27zM19 14.9L14.9 19H9.1L5 14.9V9.1L9.1 5h5.8L19 9.1z"/><path fill="currentColor" d="M11 7h2v6h-2zm0 8h2v2h-2z"/></svg>`