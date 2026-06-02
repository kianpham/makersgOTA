import { useState } from 'react'
import Card, { CardHead } from '../components/Card'
import Button from '../components/Button'
import { API_BASE, api } from '../api/client'

export default function Settings() {
  const [test, setTest] = useState(null)
  const [oldPw, setOldPw] = useState('')
  const [newPw, setNewPw] = useState('')
  const [confirmPw, setConfirmPw] = useState('')
  const [pwMsg, setPwMsg] = useState(null)
  const [pwBusy, setPwBusy] = useState(false)

  async function testConn() {
    setTest('Đang kiểm tra…')
    try {
      const devices = await api.listDevices()
      setTest(`OK — ${devices.length} thiết bị`)
    } catch (e) {
      setTest('Lỗi: ' + e.message)
    }
  }

  async function changePassword(e) {
    e.preventDefault()
    setPwMsg(null)
    if (newPw.length < 4) {
      setPwMsg({ ok: false, text: 'Mật khẩu mới phải ≥ 4 ký tự.' })
      return
    }
    if (newPw !== confirmPw) {
      setPwMsg({ ok: false, text: 'Xác nhận mật khẩu không khớp.' })
      return
    }
    setPwBusy(true)
    try {
      await api.changePassword(oldPw, newPw)
      setPwMsg({ ok: true, text: 'Đã đổi mật khẩu.' })
      setOldPw('')
      setNewPw('')
      setConfirmPw('')
    } catch (err) {
      setPwMsg({
        ok: false,
        text: err.message.includes('400') ? 'Mật khẩu cũ không đúng.' : 'Lỗi: ' + err.message,
      })
    } finally {
      setPwBusy(false)
    }
  }

  return (
    <div className="page-enter">
      <Card>
        <CardHead icon="ti-key" title="Đổi mật khẩu" />
        <form
          onSubmit={changePassword}
          style={{ display: 'flex', flexDirection: 'column', gap: 12, maxWidth: 360 }}
        >
          <div className="field">
            <label className="field__label">Mật khẩu hiện tại</label>
            <input className="input" type="password" value={oldPw} onChange={(e) => setOldPw(e.target.value)} />
          </div>
          <div className="field">
            <label className="field__label">Mật khẩu mới</label>
            <input className="input" type="password" value={newPw} onChange={(e) => setNewPw(e.target.value)} />
          </div>
          <div className="field">
            <label className="field__label">Xác nhận mật khẩu mới</label>
            <input
              className="input"
              type="password"
              value={confirmPw}
              onChange={(e) => setConfirmPw(e.target.value)}
            />
          </div>
          {pwMsg && (
            <div style={{ fontSize: 12, color: pwMsg.ok ? 'var(--success-fg)' : 'var(--danger-fg)' }}>
              {pwMsg.text}
            </div>
          )}
          <Button variant="primary" icon="ti-device-floppy" disabled={pwBusy}>
            {pwBusy ? 'Đang lưu…' : 'Đổi mật khẩu'}
          </Button>
        </form>
      </Card>

      <div style={{ height: 16 }} />
      <Card>
        <CardHead icon="ti-plug" title="Kết nối cloud" />
        <div className="field" style={{ maxWidth: 460 }}>
          <label className="field__label">API base</label>
          <input className="input mono" value={API_BASE} readOnly />
        </div>
        <p className="muted" style={{ fontSize: 12, marginTop: 12 }}>
          Đổi bằng biến môi trường <span className="mono">VITE_API_BASE</span> (xem .env.example). Khi
          dev, <span className="mono">/api/v1</span> được proxy sang http://127.0.0.1:8000.
        </p>
        <div style={{ marginTop: 16, display: 'flex', gap: 12, alignItems: 'center' }}>
          <Button variant="secondary" icon="ti-activity" onClick={testConn}>
            Kiểm tra kết nối
          </Button>
          {test && (
            <span className="muted" style={{ fontSize: 13 }}>
              {test}
            </span>
          )}
        </div>
      </Card>

      <div style={{ height: 16 }} />

      <div className="info-banner">
        <div className="info-banner__title">
          <i className="ti ti-info-circle" />
          Về control panel
        </div>
        Giao diện theo style DESIGN.md, nội dung map với cloud Phase 1 (OTA + devices + firmware +
        WebSocket). Hợp đồng: PHASE1.md.
      </div>
    </div>
  )
}
