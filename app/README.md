# app/ — MakerSG Control Panel (web · board ES3C35P)

Web control panel cho dự án IoT, **style lấy từ DESIGN.md** (industrial-utilitarian, song ngữ Anh-Việt), **nội dung map với cloud Phase 1** ([../PHASE1.md](../PHASE1.md), [../cloud/](../cloud/)).
Stack: **Vite + React + CSS thuần** (design tokens). Icon Tabler webfont. Font DM Sans + JetBrains Mono.

> Thay cho hướng app Flutter ban đầu — pivot sang web dashboard ở Phase 1.

## Chạy

```bash
# 1. Bật cloud (terminal khác)
cd ../cloud && uvicorn app.main:app --host 0.0.0.0 --port 8000

# 2. App
npm install
npm run dev        # http://localhost:5173 (proxy /api → cloud:8000)
npm run build      # → dist/
```

API base mặc định `/api/v1` (proxy sang cloud khi dev). Deploy tách host: đặt `VITE_API_BASE` (xem `.env.example`); cloud đã bật CORS.

## Đăng nhập

Console yêu cầu **đăng nhập admin** (mặc định `admin` / `admin` — đổi qua `XZ_ADMIN_USER`/`XZ_ADMIN_PASSWORD` ở cloud).
Token lưu localStorage, gắn `Authorization: Bearer` vào mọi call quản lý. Hết hạn / 401 → tự đăng xuất về màn Login. Nút **Đăng xuất** ở góc phải header.

## Trang (map với cloud)

| Trang | Cloud endpoint | Chức năng |
|---|---|---|
| Login | `POST /auth/login` | Đăng nhập admin |
| Dashboard | `GET /devices`, `GET /firmware` | Thống kê thiết bị/firmware + recent devices |
| Devices | `GET /devices`, `POST /devices/register` | Danh sách thiết bị; đăng ký thiết bị pending bằng mã kích hoạt |
| Firmware | `GET /firmware`, `POST /firmware/upload` | Liệt kê + upload firmware OTA |
| Settings | (test `GET /devices`) | API base + kiểm tra kết nối |

Khi cloud chưa chạy, mỗi trang hiện banner "Không kết nối được cloud" + hướng dẫn.

## Cấu trúc

```
src/
├── main.jsx · App.jsx        # shell + chuyển trang (state, chưa cần react-router)
├── styles/                   # tokens.css · global.css · components.css  (STYLE từ DESIGN.md)
├── api/client.js             # gọi cloud (API_BASE = VITE_API_BASE || '/api/v1')
├── hooks/useAsync.js         # fetch {loading,data,error,reload}
├── utils/format.js           # fmtTime / fmtBytes / shortHash
├── data/nav.js               # điều hướng song ngữ
├── components/               # Card · Badge · Button · StatCard · Sidebar · CloudDown
└── pages/                    # Dashboard · Devices · Firmware · Settings
```

## Mở rộng

Thêm trang: tạo `src/pages/<Page>.jsx`, thêm vào `data/nav.js` + map trong `App.jsx` (gắn react-router khi cần URL).
Phase sau: realtime WebSocket trạng thái thiết bị, log, điều khiển lệnh (khi cloud có audio/command).

> Style giữ nguyên DESIGN.md; chỉ nội dung là IoT. Các file `.jsx` mẫu DESIGN.md nhắc tới không có sẵn — component dựng từ spec.
