import Card from './Card'

const TONE = {
  primary: { bg: 'var(--primary-100)', fg: 'var(--primary-600)' },
  info: { bg: 'var(--info-bg)', fg: 'var(--info-fg)' },
  success: { bg: 'var(--success-bg)', fg: 'var(--success-fg)' },
  danger: { bg: 'var(--danger-bg)', fg: 'var(--danger-fg)' },
  warning: { bg: 'var(--warning-bg)', fg: 'var(--warning-fg)' },
}

export default function StatCard({ icon, value, label, labelVi, sub, tone = 'primary' }) {
  const t = TONE[tone] || TONE.primary
  return (
    <Card>
      <div className="statcard">
        <div className="statcard__icon" style={{ background: t.bg, color: t.fg }}>
          <i className={`ti ${icon}`} />
        </div>
        <div>
          <div className="statcard__value">{value}</div>
          <div className="statcard__label">
            {label}
            {labelVi ? ` · ${labelVi}` : ''}
          </div>
          {sub && <div className="statcard__sub">{sub}</div>}
        </div>
      </div>
    </Card>
  )
}
