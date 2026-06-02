import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// Dev proxy: chuyển /api và /healthz sang cloud FastAPI (mặc định cổng 8000) — tránh CORS khi dev.
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/api': 'http://127.0.0.1:8000',
      '/healthz': 'http://127.0.0.1:8000',
    },
  },
})
