import { useState } from 'react'
import Card, { CardHead } from '../components/Card'
import Button from '../components/Button'
import CloudDown from '../components/CloudDown'
import { api } from '../api/client'
import { useAsync } from '../hooks/useAsync'
import { fmtTime } from '../utils/format'

export default function Admins() {
  const { loading, data, error, reload } = useAsync(api.listAdmins, [])
  const [username, setUsername] = useState('')
  const [password, setPassword] = useState('')
  const [busy, setBusy] = useState(false)
  const [msg, setMsg] = useState(null)

  if (error) return <CloudDown error={error} onRetry={reload} />
  const admins = data || []

  async function addAdmin(e) {
    e.preventDefault()
    setMsg(null)
    if (!username.trim() || password.length < 4) {
      setMsg('Nhập username và mật khẩu ≥ 4 ký tự.')
      return
    }
    setBusy(true)
    try {
      await api.createAdmin(username.trim(), password)
      setUsername('')
      setPassword('')
      reload()
    } catch (err) {
      setMsg(err.message.includes('409') ? 'Username đã tồn tại.' : 'Lỗi: ' + err.message)
    } finally {
      setBusy(false)
    }
  }

  async function removeAdmin(name) {
    setMsg(null)
    try {
      await api.deleteAdmin(name)
      reload()
    } catch (err) {
      setMsg(err.message.includes('400') ? 'Không thể xoá admin cuối cùng.' : 'Lỗi: ' + err.message)
    }
  }

  return (
    <div className="page-enter">
      <Card>
        <CardHead icon="ti-user-plus" title="Thêm quản trị viên" />
        <form className="form-grid" onSubmit={addAdmin}>
          <div className="field">
            <label className="field__label">Username</label>
            <input className="input" value={username} onChange={(e) => setUsername(e.target.value)} />
          </div>
          <div className="field">
            <label className="field__label">Mật khẩu (≥ 4)</label>
            <input
              className="input"
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
            />
          </div>
          <div />
          <Button variant="primary" icon="ti-plus" disabled={busy}>
            {busy ? 'Đang tạo…' : 'Tạo'}
          </Button>
        </form>
        {msg && <div style={{ color: 'var(--danger-fg)', fontSize: 12, marginTop: 12 }}>{msg}</div>}
      </Card>

      <div style={{ height: 16 }} />

      <Card pad0>
        <div style={{ padding: '20px 20px 0' }}>
          <CardHead
            icon="ti-users"
            title="Quản trị viên"
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
                <th>Username</th>
                <th>Tạo lúc</th>
                <th></th>
              </tr>
            </thead>
            <tbody>
              {loading && (
                <tr>
                  <td colSpan={3}>
                    <div className="skeleton" />
                  </td>
                </tr>
              )}
              {!loading &&
                admins.map((a) => (
                  <tr key={a.username}>
                    <td className="mono">{a.username}</td>
                    <td className="muted">{fmtTime(a.created)}</td>
                    <td style={{ textAlign: 'right' }}>
                      <Button
                        size="sm"
                        variant="danger"
                        icon="ti-trash"
                        onClick={() => removeAdmin(a.username)}
                        disabled={admins.length <= 1}
                      >
                        Xoá
                      </Button>
                    </td>
                  </tr>
                ))}
            </tbody>
          </table>
        </div>
      </Card>
    </div>
  )
}
