import { useState } from 'react'
import Card from '../components/Card'
import Badge from '../components/Badge'
import Button from '../components/Button'
import CloudDown from '../components/CloudDown'
import { api } from '../api/client'
import { useAsync } from '../hooks/useAsync'
import { fmtTime } from '../utils/format'

export default function Devices() {
  const { loading, data, error, reload } = useAsync(api.listDevices, [], { pollMs: 5000 })
  const [editing, setEditing] = useState(null) // device_id đang đăng ký
  const [code, setCode] = useState('')
  const [msg, setMsg] = useState(null)

  if (error) return <CloudDown error={error} onRetry={reload} />
  const devices = data || []

  async function submitRegister(device_id) {
    setMsg(null)
    const r = await api.registerDevice(device_id, code.trim())
    if (r.status === 'success') {
      setEditing(null)
      setCode('')
      reload()
    } else {
      setMsg(r.status === 'pending' ? 'Mã không đúng hoặc đã hết hạn.' : 'Không đăng ký được thiết bị.')
    }
  }

  return (
    <div className="page-enter">
      <div className="toolbar">
        <Button variant="secondary" size="sm" icon="ti-refresh" onClick={reload}>
          Làm mới
        </Button>
        <div className="toolbar__spacer" />
        <span className="muted" style={{ fontSize: 12 }}>
          {devices.length} thiết bị
        </span>
      </div>

      <Card pad0>
        <div style={{ padding: '8px 12px' }}>
          <table className="table">
            <thead>
              <tr>
                <th>Device ID</th>
                <th>Board</th>
                <th>Firmware</th>
                <th>Status</th>
                <th>Conn</th>
                <th>Last seen</th>
                <th></th>
              </tr>
            </thead>
            <tbody>
              {loading &&
                [0, 1, 2].map((i) => (
                  <tr key={i}>
                    <td colSpan={7}>
                      <div className="skeleton" />
                    </td>
                  </tr>
                ))}
              {!loading && devices.length === 0 && (
                <tr>
                  <td colSpan={7}>
                    <div className="empty">
                      <i className="ti ti-device-desktop-off" />
                      Chưa có thiết bị. Sẽ xuất hiện sau lần OTA check-in đầu tiên.
                    </div>
                  </td>
                </tr>
              )}
              {!loading &&
                devices.map((d) => (
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
                    <td style={{ textAlign: 'right' }}>
                      {!d.registered && editing !== d.device_id && (
                        <Button
                          size="sm"
                          variant="ghost"
                          icon="ti-key"
                          onClick={() => {
                            setEditing(d.device_id)
                            setCode('')
                            setMsg(null)
                          }}
                        >
                          Đăng ký
                        </Button>
                      )}
                      {!d.registered && editing === d.device_id && (
                        <span style={{ display: 'inline-flex', gap: 8, alignItems: 'center' }}>
                          <input
                            className="input mono"
                            style={{ width: 110 }}
                            placeholder="Mã 6 số"
                            value={code}
                            onChange={(e) => setCode(e.target.value)}
                          />
                          <Button size="sm" variant="primary" onClick={() => submitRegister(d.device_id)}>
                            OK
                          </Button>
                          <Button size="sm" variant="secondary" onClick={() => setEditing(null)}>
                            Hủy
                          </Button>
                        </span>
                      )}
                    </td>
                  </tr>
                ))}
            </tbody>
          </table>
        </div>
      </Card>

      {msg && (
        <div className="banner-down" style={{ marginTop: 16 }}>
          {msg}
        </div>
      )}
    </div>
  )
}
