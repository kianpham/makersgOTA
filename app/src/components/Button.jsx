export default function Button({ variant = 'secondary', size, block, icon, children, ...rest }) {
  const cls = ['btn', `btn--${variant}`, size === 'sm' && 'btn--sm', block && 'btn--block']
    .filter(Boolean)
    .join(' ')
  return (
    <button className={cls} {...rest}>
      {icon && <i className={`ti ${icon}`} />}
      {children}
    </button>
  )
}
