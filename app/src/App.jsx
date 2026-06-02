import { useEffect, useState } from 'react'
import Sidebar from './components/Sidebar'
import Login from './pages/Login'
import Dashboard from './pages/Dashboard'
import Devices from './pages/Devices'
import Firmware from './pages/Firmware'
import Admins from './pages/Admins'
import Settings from './pages/Settings'
import Button from './components/Button'
import { PAGE_TITLES } from './data/nav'
import { getToken, setToken, setOnUnauthorized } from './api/client'

const PAGES = {
  dashboard: Dashboard,
  devices: Devices,
  firmware: Firmware,
  admins: Admins,
  settings: Settings,
}

export default function App() {
  const [token, setTok] = useState(getToken())
  const [active, setActive] = useState('dashboard')
  const [collapsed, setCollapsed] = useState(false)

  // 401 từ bất kỳ call quản lý nào → tự đăng xuất, quay về Login.
  useEffect(() => {
    setOnUnauthorized(() => setTok(''))
  }, [])

  if (!token) {
    return <Login onLogin={() => setTok(getToken())} />
  }

  function logout() {
    setToken(null)
    setTok('')
  }

  const title = PAGE_TITLES[active]
  const Page = PAGES[active] || Dashboard

  return (
    <div className="appshell">
      <Sidebar
        active={active}
        onSelect={setActive}
        collapsed={collapsed}
        onToggle={() => setCollapsed((c) => !c)}
      />
      <main className="content">
        <div className="page-header">
          <div className="page-header__main">
            <div className="page-title">{title?.en}</div>
            <div className="page-subtitle">{title?.vi}</div>
          </div>
          <Button variant="secondary" size="sm" icon="ti-logout" onClick={logout}>
            Đăng xuất
          </Button>
        </div>
        <Page />
      </main>
    </div>
  )
}
