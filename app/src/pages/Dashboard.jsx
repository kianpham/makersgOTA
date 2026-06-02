import Card, { CardHead } from '../components/Card'
import StatCard from '../components/StatCard'
import Badge from '../components/Badge'
import Button from '../components/Button'
import CloudDown from '../components/CloudDown'
import { api } from '../api/client'
import { useAsync } from '../hooks/useAsync'
import { fmtTime } from '../utils/format'

export default function Dashboard() {
  const { loading, data, error, reload } = useAsync(
    () =>
      Promise.all([api.listDevices(), api.listFirmwares()]).then(([devices, firmwares]) => ({
        devices,
        firmwares,
      })),
    [],
    { pollMs: 5000 }, // auto-refresh online status
  )

  if (error) return <CloudDown error={error} onRetry={reload} />

  const devices = data?.devices || []
  const firmwares = data?.firmwares || []
  const registered = devices.filter((d) => d.registered).length
  const online = devices.filter((d) => d.online).length
  const recent = [...devices].sort((a, b) => (b.last_seen || 0) - (a.last_seen || 0)).slice(0, 6)

  const stats = [
    { icon: 'ti-devices-2', value: loading ? '—' : devices.length, label: 'Devices', labelVi: 'Thiết bị', tone: 'primary', sub: 'tổng đã thấy' },
    { icon: 'ti-wifi', value: loading ? '—' : online, label: 'Online', labelVi: 'Đang kết nối', tone: 'success', sub: 'qua WebSocket' },
    { icon: 'ti-circle-check', value: loading ? '—' : registered, label: 'Registered', labelVi: 'Đã đăng ký', tone: 'info' },
    { icon: 'ti-package', value: loading ? '—' : firmwares.length, label: 'Firmware', labelVi: 'Bản firmware', tone: 'warning' },
  ]

  return (
    <div className="page-enter">
      <div className="stats-grid">
        {stats.map((s) => (
          <StatCard key={s.label} {...s} />
        ))}
      </div>

      <div className="dash-main">
        <Card pad0>
          <div style={{ padding: '20px 20px 0' }}>
            <CardHead
              icon="ti-history"
              title="Recent Devices"
              right={
                <Button size="sm" variant="ghost" icon="ti-refresh" onClick={reload}>
                  Làm mới
                </Button>
              }
            />
          </div>
          <div style={{ padding: '4px 20px 12px' }}>
            <table className="table">
              <thead>
                <tr>
                  <th>Device ID</th>
                  <th>Board</th>
                  <th>Firmware</th>
                  <th>Status</th>
                  <th>Conn</th>
                  <th>Last seen</th>
                </tr>
              </thead>
              <tbody>
                {loading &&
                  [0, 1, 2].map((i) => (
                    <tr key={i}>
                      <td colSpan={6}>
                        <div className="skeleton" />
                      </td>
                    </tr>
                  ))}
                {!loading && recent.length === 0 && (
                  <tr>
                    <td colSpan={6}>
                      <div className="empty">
                        <i className="ti ti-device-desktop-off" />
                        Chưa có thiết bị nào check-in
                      </div>
                    </td>
                  </tr>
                )}
                {!loading &&
                  recent.map((d) => (
                    <tr key={d.device_id}>
                      <td className="mono">{d.device_id}</td>
                      <td>{d.board || '—'}</td>
                      <td className="mono">{d.app_version || '—'}</td>
                      <td>
                        <Badge status={d.registered ? 'registered' : 'pending'} />
                      </td>
                      <td>
                        <Badge status={d.online ? 'online' : 'offline'} />
                      </td>
                      <td className="muted">{fmtTime(d.last_seen)}</td>
                    </tr>
                  ))}
              </tbody>
            </table>
          </div>
        </Card>

        <div className="dash-side">
          <Card>
            <CardHead icon="ti-bolt" title="Quick Actions" />
            <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
              <Button variant="secondary" block icon="ti-refresh" onClick={reload}>
                Làm mới dữ liệu
              </Button>
            </div>
          </Card>

          <div className="info-banner">
            <div className="info-banner__title">
              <i className="ti ti-route" />
              Phase 1 — Connect
            </div>
            Luồng: thiết bị check-in OTA → hiện mã kích hoạt → đăng ký ở tab Devices → kết nối WebSocket.
          </div>
        </div>
      </div>
    </div>
  )
}
