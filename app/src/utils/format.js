export function fmtTime(epochSec) {
  if (!epochSec) return '—'
  const d = new Date(epochSec * 1000)
  return d.toLocaleString('vi-VN', { hour12: false })
}

export function shortHash(h, n = 12) {
  return h ? h.slice(0, n) + '…' : '—'
}

export function fmtBytes(b) {
  if (b === null || b === undefined) return '—'
  if (b < 1024) return b + ' B'
  if (b < 1024 * 1024) return (b / 1024).toFixed(1) + ' KB'
  return (b / 1024 / 1024).toFixed(2) + ' MB'
}
