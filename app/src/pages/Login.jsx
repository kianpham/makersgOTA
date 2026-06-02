import { useState } from 'react'
import Button from '../components/Button'
import { api, setToken } from '../api/client'

export default function Login({ onLogin }) {
  const [username, setUsername] = useState('admin')
  const [password, setPassword] = useState('')
  const [err, setErr] = useState(null)
  const [busy, setBusy] = useState(false)

  async function submit(e) {
    e.preventDefault()
    setBusy(true)
    setErr(null)
    try {
      const { token, username: u } = await api.login(username.trim(), password)
      setToken(token)
      onLogin(u)
    } catch (e) {
      setErr(e.message)
    } finally {
      setBusy(false)
    }
  }

  return (
    <div style={{ display: 'grid', placeItems: 'center', height: '100vh' }} className="page-enter">
      <div className="card" style={{ width: 360 }}>
        <div style={{ display: 'flex', alignItems: 'center', gap: 12, marginBottom: 16 }}>
          <div className="sidebar__logo">
            <i className="ti ti-cpu" />
          </div>
          <div>
            <div style={{ fontWeight: 700, fontSize: 16, color: 'var(--gray-900)' }}>
              MakerSG Console
            </div>
            <div className="muted" style={{ fontSize: 12 }}>
              Đăng nhập quản trị
            </div>
          </div>
        </div>
        <form onSubmit={submit} style={{ display: 'flex', flexDirection: 'column', gap: 12 }}>
          <div className="field">
            <label className="field__label">Tài khoản</label>
            <input
              className="input"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              autoFocus
            />
          </div>
          <div className="field">
            <label className="field__label">Mật khẩu</label>
            <input
              className="input"
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
            />
          </div>
          {err && <div style={{ color: 'var(--danger-fg)', fontSize: 12 }}>{err}</div>}
          <Button variant="primary" icon="ti-login" disabled={busy}>
            {busy ? 'Đang đăng nhập…' : 'Đăng nhập'}
          </Button>
        </form>
      </div>
    </div>
  )
}
