// Status badge — tái dùng bảng màu DESIGN.md cho trạng thái thiết bị/firmware.
const MAP = {
  registered: { cls: 'badge--approved', label: 'Registered', icon: 'ti-circle-check' },
  pending: { cls: 'badge--draft', label: 'Pending', icon: 'ti-clock' },
  online: { cls: 'badge--approved', label: 'Online', icon: 'ti-wifi' },
  offline: { cls: 'badge--obsolete', label: 'Offline', icon: 'ti-wifi-off' },
  current: { cls: 'badge--released', label: 'Latest', icon: 'ti-star' },
}

export default function Badge({ status, label }) {
  const m = MAP[status] || MAP.pending
  return (
    <span className={`badge ${m.cls}`}>
      <i className={`ti ${m.icon}`} />
      {label || m.label}
    </span>
  )
}
