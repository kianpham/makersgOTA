# cloud/ — MakerSG Server Phase 1 (OTA + WebSocket)

FastAPI server cho lớp kết nối lõi: OTA check, activation, firmware download, WebSocket handshake.
Hợp đồng đầy đủ: [../PHASE1.md](../PHASE1.md). Stack/quy ước: [../iot-cloud-module/SKILL.md](../iot-cloud-module/SKILL.md).

> Phase 1 default: WebSocket-only (chưa MQTT), SQLite device store, HMAC token, firmware file + manifest.

## Cài & chạy

```bash
cd cloud
python -m venv .venv
.venv/Scripts/activate          # Windows: .venv\Scripts\Activate.ps1   |  *nix: source .venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --host 0.0.0.0 --port 8000
```

Khi test với board thật: đặt `XZ_PUBLIC_HOST` = IP LAN của máy chạy server (để URL ws/firmware thiết bị gọi được).

## Test

```bash
cd cloud
python -m pytest            # cần .venv đã cài requirements
```

## Thử bằng curl (giả lập thiết bị)

```bash
# 1. Thiết bị lạ check-in → nhận mã activation
curl -s -X POST http://127.0.0.1:8000/api/v1/ota \
  -H "device-id: AA:BB:CC:DD:EE:01" -H "content-type: application/json" \
  -d '{"application":{"version":"1.0.0"},"board":{"type":"esp32s3","name":"es3c35p"}}'

# 2. Đăng ký thiết bị bằng mã (Phase 1 thay cho app)
curl -s -X POST http://127.0.0.1:8000/api/v1/devices/register \
  -H "content-type: application/json" \
  -d '{"device_id":"AA:BB:CC:DD:EE:01","code":"<MÃ_Ở_BƯỚC_1>"}'

# 3. OTA lại → nhận websocket.url + token (+ firmware nếu đã upload)
curl -s -X POST http://127.0.0.1:8000/api/v1/ota \
  -H "device-id: AA:BB:CC:DD:EE:01" -H "content-type: application/json" \
  -d '{"application":{"version":"1.0.0"},"board":{"name":"es3c35p"}}'

# 4. (tùy chọn) Upload firmware bản mới
curl -s -X POST http://127.0.0.1:8000/api/v1/firmware/upload \
  -F version=1.1.0 -F board=es3c35p -F file=@firmware.bin
```

## Endpoint

| Method | Path | Auth | Mô tả |
|---|---|---|---|
| POST | `/api/v1/auth/login` | — | Login admin → `{ token, username }` |
| GET | `/api/v1/auth/me` | admin | Thông tin admin hiện tại |
| POST | `/api/v1/auth/change-password` | admin | Đổi mật khẩu của chính mình |
| GET | `/api/v1/admins` | **admin** | Liệt kê tài khoản admin |
| POST | `/api/v1/admins` | **admin** | Tạo admin mới |
| DELETE | `/api/v1/admins/{username}` | **admin** | Xoá admin (chặn xoá admin cuối) |
| GET/POST | `/api/v1/ota` | thiết bị (mở) | OTA check → ws+token+firmware hoặc activation |
| POST | `/api/v1/ota/activate` | thiết bị (mở) | Thiết bị gửi mã kích hoạt |
| WS | `/api/v1/ws` | device token | Handshake `hello` + ping/pong |
| GET | `/api/v1/firmware/{id}/download` | thiết bị (mở) | Tải .bin (header `X-Checksum-SHA256`) |
| GET | `/api/v1/devices` | **admin** | Liệt kê thiết bị (kèm `online` — đang kết nối ws) |
| POST | `/api/v1/devices/register` | **admin** | Đăng ký thiết bị bằng mã |
| GET | `/api/v1/firmware` | **admin** | Liệt kê firmware |
| POST | `/api/v1/firmware/upload` | **admin** | Nạp firmware |
| GET | `/healthz` | — | Health check |

Admin mặc định `admin/admin` (đổi qua `XZ_ADMIN_USER` / `XZ_ADMIN_PASSWORD`). Endpoint quản lý cần header `Authorization: Bearer <token>`; endpoint thiết bị vẫn mở để firmware hoạt động.

OpenAPI docs: http://127.0.0.1:8000/docs
