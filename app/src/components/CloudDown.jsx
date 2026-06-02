import { API_BASE } from '../api/client'
import Button from './Button'

// Hiển thị khi không gọi được cloud — kèm hướng dẫn chạy server.
export default function CloudDown({ error, onRetry }) {
  return (
    <div className="page-enter">
      <div className="banner-down">
        <strong>Không kết nối được cloud.</strong> Kiểm tra server FastAPI đang chạy.
        <div style={{ marginTop: 8 }}>
          API base: <code>{API_BASE}</code>
          {error ? ` · ${error}` : ''}
        </div>
        <div style={{ marginTop: 8 }}>
          Chạy: <code>cd cloud &amp;&amp; uvicorn app.main:app --host 0.0.0.0 --port 8000</code>
        </div>
        <div style={{ marginTop: 12 }}>
          <Button variant="secondary" icon="ti-refresh" onClick={onRetry}>
            Thử lại
          </Button>
        </div>
      </div>
    </div>
  )
}
