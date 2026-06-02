// Điều hướng control panel IoT (style theo DESIGN.md, nội dung map với cloud Phase 1). Song ngữ.
export const NAV = [
  {
    group: 'OVERVIEW',
    items: [{ id: 'dashboard', en: 'Dashboard', vi: 'Tổng quan', icon: 'ti-layout-dashboard' }],
  },
  {
    group: 'FLEET',
    items: [{ id: 'devices', en: 'Devices', vi: 'Thiết bị', icon: 'ti-devices-2' }],
  },
  {
    group: 'OTA',
    items: [{ id: 'firmware', en: 'Firmware', vi: 'Firmware / OTA', icon: 'ti-package' }],
  },
  {
    group: 'SYSTEM',
    items: [
      { id: 'admins', en: 'Admins', vi: 'Quản trị viên', icon: 'ti-users' },
      { id: 'settings', en: 'Settings', vi: 'Cấu hình', icon: 'ti-settings' },
    ],
  },
]

export const PAGE_TITLES = Object.fromEntries(
  NAV.flatMap((g) => g.items).map((i) => [i.id, { en: i.en, vi: i.vi }]),
)
