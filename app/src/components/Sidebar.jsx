import { NAV } from '../data/nav'

export default function Sidebar({ active, onSelect, collapsed, onToggle }) {
  return (
    <aside className={`sidebar ${collapsed ? 'is-collapsed' : ''}`}>
      <div className="sidebar__head">
        <div className="sidebar__logo">
          <i className="ti ti-cpu" />
        </div>
        {!collapsed && <span className="sidebar__brand">MakerSG Console</span>}
      </div>

      <nav className="sidebar__nav">
        {NAV.map((g) => (
          <div key={g.group}>
            <div className="sidebar__group">{g.group}</div>
            {g.items.map((it) => (
              <button
                key={it.id}
                className={`navitem ${active === it.id ? 'is-active' : ''}`}
                onClick={() => onSelect(it.id)}
                title={`${it.en} — ${it.vi}`}
              >
                <i className={`ti ${it.icon}`} />
                {!collapsed && (
                  <span className="navitem__label bi">
                    <span className="bi__en">{it.en}</span>
                    <span className="bi__vi">{it.vi}</span>
                  </span>
                )}
              </button>
            ))}
          </div>
        ))}
      </nav>

      <div className="sidebar__foot">
        {!collapsed && <span className="sidebar__ver">Phase 1 · v0.1.0</span>}
        <button className="sidebar__toggle" onClick={onToggle} title="Thu gọn / mở rộng">
          <i
            className={`ti ${collapsed ? 'ti-layout-sidebar-left-expand' : 'ti-layout-sidebar-left-collapse'}`}
          />
        </button>
      </div>
    </aside>
  )
}
