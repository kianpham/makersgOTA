# MakerSG Phase 1 — Core OTA + WiFi provisioning + WebSocket connect

> Thay đổi triển khai: các tính năng trợ lý (F1–F13 trong [FEATURES.md](FEATURES.md)) và app **hoãn sang phase sau**. Phase 1 chỉ làm **lớp kết nối lõi** của thiết bị, đi qua hợp đồng chung cho cả cloud + firmware.
> Tham khảo giao thức: hệ Xiaozhi-ESP32 (server Python/FastAPI). Ta **viết mới trong `cloud/`** theo pattern đó, lấy giao thức làm chuẩn hợp đồng.

## Mục tiêu Phase 1

Đưa board ES3C35P từ lúc bật nguồn (chưa có WiFi) tới khi **kết nối được WebSocket của server ta** và **cập nhật firmware OTA** được:

1. **WiFi provisioning trên thiết bị** — SoftAP + captive portal nhập SSID/mật khẩu, lưu NVS.
2. **OTA check + update** — thiết bị gọi `/ota`, nhận cấu hình server + thông tin firmware; tải & flash nếu có bản mới.
3. **WebSocket connect** — thiết bị kết nối ws bằng `device-id` + token, handshake `hello`.

Default thực dụng Phase 1 (ghi rõ để chốt sau): **WebSocket-only** (bỏ MQTT), **SQLite** device store, **HMAC-SHA256** token, firmware lưu file + manifest. Không làm audio/STT/LLM/TTS, không làm app.

## Hợp đồng kết nối (cloud ↔ firmware) — nguồn sự thật Phase 1

### 1) OTA endpoint

```
POST /api/v1/ota            (GET cũng được hỗ trợ để kiểm tra nhanh)
Headers:
  device-id:  <MAC>         (bắt buộc, vd "AA:BB:CC:DD:EE:FF")
  client-id:  <uuid>        (tùy chọn, mặc định = device-id)
Body (JSON):
  {
    "application": { "version": "1.0.0" },
    "board":       { "type": "esp32s3", "name": "es3c35p" },
    "chip_model_name": "esp32s3"
  }
```

**Response — thiết bị ĐÃ đăng ký:**
```json
{
  "server_time": { "timestamp": 1730000000000, "timezone_offset": 420 },
  "websocket":  { "url": "ws://<host>:8000/api/v1/ws", "token": "<jwt-like-hmac>" },
  "firmware":   { "version": "1.1.0", "url": "http://<host>:8000/api/v1/firmware/<id>/download", "sha256": "<64-hex>" },
  "audio_params": { "format": "opus", "sample_rate": 16000, "channels": 1, "buffer_size_ms": 500 },
  "activation": null
}
```
- Nếu `firmware.version` == version thiết bị gửi → vẫn trả nhưng thiết bị **không** update.

**Response — thiết bị CHƯA đăng ký:**
```json
{
  "server_time": { ... },
  "activation": { "code": "123456", "message": "Nhập mã 123456 để kích hoạt", "timeout_ms": 300000 },
  "websocket": null, "firmware": null
}
```
- Thiết bị hiển thị `code` lên màn; người vận hành đăng ký mã đó (Phase 1: qua API/CLI; app làm sau).

### 2) Activation

```
POST /api/v1/ota/activate
Headers: device-id: <MAC>
Body: { "code": "123456" }
→ 200 { "status": "success" }   | 202 { "status": "pending" }   | 404
```
> Phase 1 đăng ký mã thủ công: `POST /api/v1/devices/register { "device_id": "<MAC>", "code": "123456" }` (hoặc CLI). Đây là chỗ "đăng nhập/đăng ký thiết bị".

### 3) Firmware download

```
GET  /api/v1/firmware/{id}/download     → octet-stream (.bin), header X-Checksum-SHA256
POST /api/v1/firmware/upload            → nạp bản mới (multipart: file + version + board)  (Phase 1: dev only)
```

### 4) WebSocket

```
WS  /api/v1/ws
Auth (header hoặc query):
  device-id:      <MAC>            (bắt buộc)
  authorization:  Bearer <token>   (token lấy từ OTA response; verify HMAC + khớp device-id)
Handshake:
  server → client: { "type": "hello", "session_id": "<uuid>", "audio_params": {...} }
  client → server: { "type": "hello", "version": 1, "transport": "websocket" }
  lỗi:             { "type": "error", "message": "..." }
Phase 1: sau hello chỉ giữ kết nối + ping/pong + log; chưa xử lý audio.
```

## Việc theo mặt

| Mặt | Skill | Việc Phase 1 |
|---|---|---|
| Cloud (FastAPI) | `/iot-cloud-module` + `/iot-e2e-module` (gác hợp đồng) | OTA endpoint, activation, device store (SQLite), firmware manifest+download, token HMAC, WebSocket handshake |
| Firmware (ESP-IDF) | `/iot-fw-module` + `/iot-e2e-module` | WiFi provisioning (SoftAP+portal), STA connect từ NVS, OTA client (`esp_https_ota`), WebSocket client (`esp_websocket_client`) + hello |

## Verify end-to-end Phase 1

1. Chạy cloud local: `uvicorn app.main:app --host 0.0.0.0 --port 8000` (trong `cloud/`).
2. Giả lập thiết bị bằng curl:
   - `POST /api/v1/ota` với `device-id` lạ → nhận `activation.code`.
   - `POST /api/v1/devices/register` mã đó → đăng ký.
   - `POST /api/v1/ota` lại → nhận `websocket` + `firmware`.
   - Kết nối ws với `device-id` + token → nhận `hello`.
3. Thiết bị thật: nạp firmware Phase 1 → SoftAP → nhập WiFi → tự OTA-check → kết nối ws (quan sát log server "device connected").
4. Test tự động: `pytest` trong `cloud/`.

## Admin console (bổ sung — quản lý)

Console quản lý (app + endpoint quản lý) yêu cầu **đăng nhập admin**:
- `POST /api/v1/auth/login {username, password}` → `{ token, username }` (HMAC bearer, role=admin).
- Endpoint **quản lý** cần `Authorization: Bearer <token>`: `GET /devices`, `POST /devices/register`, `GET /firmware`, `POST /firmware/upload`.
- Endpoint **thiết bị** vẫn MỞ (firmware gọi không cần admin): `/ota`, `/ota/activate`, `/ws`, `/firmware/{id}/download`.
- Admin mặc định `admin/admin` — đổi qua `XZ_ADMIN_USER` / `XZ_ADMIN_PASSWORD`.

> Lưu ý 2 đường đăng ký thiết bị: **thiết bị tự** qua `/ota/activate` (mở); **admin** qua `/devices/register` (cần token).

## Hoãn sang phase sau

- Audio streaming (Opus), STT/LLM/TTS, các tính năng F1–F13.
- App Flutter (toàn bộ).
- MQTT/EMQX, Redis, multi-agent — chỉ thêm nếu cần.
