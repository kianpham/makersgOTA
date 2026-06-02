// API client map tới cloud Phase 1 (FastAPI). Base mặc định '/api/v1' (đi qua vite proxy).
// Quản lý cần admin token (Bearer); endpoint thiết bị (/ota, /ws, firmware download) mở.
export const API_BASE = import.meta.env.VITE_API_BASE || '/api/v1'

let _onUnauthorized = null
export function setOnUnauthorized(fn) {
  _onUnauthorized = fn
}

export function getToken() {
  return localStorage.getItem('adminToken') || ''
}
export function setToken(t) {
  if (t) localStorage.setItem('adminToken', t)
  else localStorage.removeItem('adminToken')
}

function authHeader() {
  const t = getToken()
  return t ? { Authorization: `Bearer ${t}` } : {}
}

function handle401() {
  setToken(null)
  if (_onUnauthorized) _onUnauthorized()
  return new Error('UNAUTHORIZED')
}

async function req(path, opts = {}) {
  const res = await fetch(API_BASE + path, {
    headers: { 'content-type': 'application/json', ...authHeader() },
    ...opts,
  })
  if (res.status === 401) throw handle401()
  if (!res.ok) {
    const text = await res.text().catch(() => '')
    throw new Error(`HTTP ${res.status}${text ? ': ' + text : ''}`)
  }
  const ct = res.headers.get('content-type') || ''
  return ct.includes('json') ? res.json() : res.text()
}

export const api = {
  // POST /auth/login → { token, username }
  login: async (username, password) => {
    const res = await fetch(API_BASE + '/auth/login', {
      method: 'POST',
      headers: { 'content-type': 'application/json' },
      body: JSON.stringify({ username, password }),
    })
    if (res.status === 401) throw new Error('Sai tài khoản hoặc mật khẩu')
    if (!res.ok) throw new Error(`HTTP ${res.status}`)
    return res.json()
  },

  me: () => req('/auth/me'),

  changePassword: (old_password, new_password) =>
    req('/auth/change-password', {
      method: 'POST',
      body: JSON.stringify({ old_password, new_password }),
    }),

  listAdmins: () => req('/admins').then((d) => d.admins || []),
  createAdmin: (username, password) =>
    req('/admins', { method: 'POST', body: JSON.stringify({ username, password }) }),
  deleteAdmin: (username) =>
    req('/admins/' + encodeURIComponent(username), { method: 'DELETE' }),

  listDevices: () => req('/devices').then((d) => d.devices || []),

  registerDevice: async (device_id, code) => {
    const res = await fetch(API_BASE + '/devices/register', {
      method: 'POST',
      headers: { 'content-type': 'application/json', ...authHeader() },
      body: JSON.stringify({ device_id, code }),
    })
    if (res.status === 401) throw handle401()
    return res.json().catch(() => ({ status: 'error' }))
  },

  listFirmwares: () => req('/firmware').then((d) => d.firmwares || []),

  uploadFirmware: async (file, version, board) => {
    const fd = new FormData()
    fd.append('file', file)
    fd.append('version', version)
    if (board) fd.append('board', board)
    const res = await fetch(API_BASE + '/firmware/upload', {
      method: 'POST',
      headers: { ...authHeader() },
      body: fd,
    })
    if (res.status === 401) throw handle401()
    if (!res.ok) throw new Error(`upload HTTP ${res.status}`)
    return res.json()
  },
}
