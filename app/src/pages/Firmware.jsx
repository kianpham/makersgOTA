import { useState } from 'react'
import Card, { CardHead } from '../components/Card'
import Badge from '../components/Badge'
import Button from '../components/Button'
import CloudDown from '../components/CloudDown'
import { api } from '../api/client'
import { useAsync } from '../hooks/useAsync'
import { fmtTime, fmtBytes, shortHash } from '../utils/format'

export default function Firmware() {
  const { loading, data, error, reload } = useAsync(api.listFirmwares, [])
  const [file, setFile] = useState(null)
  const [version, setVersion] = useState('')
  const [board, setBoard] = useState('es3c35p')
  const [busy, setBusy] = useState(false)
  const [msg, setMsg] = useState(null)
  const [deleting, setDeleting] = useState(null)

  if (error) return <CloudDown error={error} onRetry={reload} />
  const firmwares = data || []

  async function upload(e) {
    e.preventDefault()
    if (!file || !version) {
      setMsg('Chọn file và nhập version.')
      return
    }
    setBusy(true)
    setMsg(null)
    try {
      await api.uploadFirmware(file, version.trim(), board.trim())
      setFile(null)
      setVersion('')
      e.target.reset?.()
      reload()
    } catch (err) {
      setMsg('Upload lỗi: ' + err.message)
    } finally {
      setBusy(false)
    }
  }

  async function del(f) {
    if (!window.confirm(`Xóa firmware ${f.version} (${f.board || '—'})?`)) return
    setDeleting(f.id)
    setMsg(null)
    try {
      await api.deleteFirmware(f.id)
      reload()
    } catch (err) {
      setMsg('Xóa lỗi: ' + err.message)
    } finally {
      setDeleting(null)
    }
  }

  return (
    <div className="page-enter">
      <Card>
        <CardHead icon="ti-upload" title="Upload firmware" />
        <form className="form-grid" onSubmit={upload}>
          <div className="field">
            <label className="field__label">File (.bin)</label>
            <input
              className="input"
              type="file"
              accept=".bin"
              onChange={(e) => setFile(e.target.files[0] || null)}
            />
          </div>
          <div className="field">
            <label className="field__label">Version</label>
            <input
              className="input mono"
              placeholder="1.1.0"
              value={version}
              onChange={(e) => setVersion(e.target.value)}
            />
          </div>
          <div className="field">
            <label className="field__label">Board</label>
            <input className="input" value={board} onChange={(e) => setBoard(e.target.value)} />
          </div>
          <Button variant="primary" icon="ti-cloud-upload" disabled={busy}>
            {busy ? 'Đang tải…' : 'Upload'}
          </Button>
        </form>
        {msg && <div style={{ color: 'var(--danger-fg)', fontSize: 12, marginTop: 12 }}>{msg}</div>}
      </Card>

      <div style={{ height: 16 }} />

      <Card pad0>
        <div style={{ padding: '20px 20px 0' }}>
          <CardHead
            icon="ti-package"
            title="Firmware versions"
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
                <th>Version</th>
                <th>Board</th>
                <th>SHA-256</th>
                <th>Size</th>
                <th>Uploaded</th>
                <th></th>
              </tr>
            </thead>
            <tbody>
              {loading &&
                [0, 1].map((i) => (
                  <tr key={i}>
                    <td colSpan={6}>
                      <div className="skeleton" />
                    </td>
                  </tr>
                ))}
              {!loading && firmwares.length === 0 && (
                <tr>
                  <td colSpan={6}>
                    <div className="empty">
                      <i className="ti ti-package-off" />
                      Chưa có firmware. Upload bản đầu tiên ở trên.
                    </div>
                  </td>
                </tr>
              )}
              {!loading &&
                firmwares.map((f, idx) => (
                  <tr key={f.id}>
                    <td className="mono">
                      {f.version}
                      {idx === 0 && (
                        <span style={{ marginLeft: 8 }}>
                          <Badge status="current" />
                        </span>
                      )}
                    </td>
                    <td>{f.board || '—'}</td>
                    <td className="mono" title={f.sha256}>
                      {shortHash(f.sha256)}
                    </td>
                    <td className="muted">{fmtBytes(f.size)}</td>
                    <td className="muted">{fmtTime(f.uploaded)}</td>
                    <td style={{ textAlign: 'right' }}>
                      <Button
                        size="sm"
                        variant="ghost"
                        icon="ti-trash"
                        onClick={() => del(f)}
                        disabled={deleting === f.id}
                      >
                        {deleting === f.id ? 'Đang xóa…' : 'Xóa'}
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
