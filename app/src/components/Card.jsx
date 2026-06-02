export default function Card({ interactive, pad0, className = '', children }) {
  const cls = ['card', interactive && 'card--interactive', pad0 && 'card--pad0', className]
    .filter(Boolean)
    .join(' ')
  return <div className={cls}>{children}</div>
}

export function CardHead({ icon, title, right }) {
  return (
    <div className="card__head">
      {icon && <i className={`ti ${icon} card__icon`} />}
      <span className="card__title">{title}</span>
      {right && <span style={{ marginLeft: 'auto' }}>{right}</span>}
    </div>
  )
}
