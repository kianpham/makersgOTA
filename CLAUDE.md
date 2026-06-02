# MakerSG — context cho agent

Sản phẩm IoT gồm ba mặt. Khi nhận một tác vụ, xác định nó đụng mặt nào rồi chọn skill.

## Bản đồ repo

```
firmware/          # ESP-IDF (target esp32s3) cho board LCDWIKI ES3C35P. PHASE 1 đã có: provisioning WiFi
                   #   (SoftAP) + OTA client + WebSocket client (main/). Màn/audio/touch/LED để phase sau.
                   #   Pin-map: .claude/skills/iot-fw-module/references/board-es3c35p.md.
cloud/             # Backend Python FastAPI. PHASE 1 đã có (chạy + 6/6 test pass): OTA + activation +
                   #   firmware download + WebSocket handshake + token HMAC + SQLite store. WebSocket-only (chưa MQTT).
app/               # Control panel web — Vite + React + CSS thuần, style theo DESIGN.md, map với cloud
                   #   (Dashboard/Devices/Firmware/Settings). PIVOT từ Flutter sang web ở Phase 1. Build sạch.
```

## Trạng thái phase

Phase hiện tại: **Phase 1 — core OTA + WiFi provisioning + WebSocket** (xem [PHASE1.md](PHASE1.md)).
Hợp đồng kết nối cloud↔firmware ở PHASE1.md là nguồn sự thật. Tính năng trợ lý F1–F13 ([FEATURES.md](FEATURES.md)) và app hoãn sang phase sau.

## Định tuyến skill

| Tác vụ | Skill |
|---|---|
| Tính năng đụng cả app + cloud + firmware (vd thêm lệnh điều khiển mới) | `/iot-e2e-module` |
| Chỉ firmware (driver, peripheral, lớp comms, tool thiết bị server không gọi) | `/iot-fw-module` |
| Chỉ cloud (endpoint, builder cho lệnh đã có, handler sự kiện, data model) | `/iot-cloud-module` |
| Chỉ app (màn hình, API client, action UI) | `/iot-app-module` |
| Bring-up board mới / setup hạ tầng một lần | Không skill — tạo project ESP-IDF (`idf.py create-project`), `idf.py set-target esp32s3`, cấu hình PSRAM OPI + flash 16MB trong `menuconfig`, copy ví dụ LCDWIKI làm tham chiếu. TODO(team): chốt cấu trúc component. |

Quy tắc vàng: nếu một tính năng cần app, cloud, firmware **cùng đồng ý một tên lệnh + schema**, luôn đi qua `/iot-e2e-module` để hợp đồng 3 chiều được gác. Đừng làm rời từng mặt rồi ghép tay.

## Quy ước chung toàn repo

- Code style + linter mỗi mặt: firmware **clang-format** (`idf.py clang-format` hoặc `clang-format -i`); cloud **ruff + black** (+ `pytest`); app **`dart format` + `flutter analyze`** (+ `flutter test`). Luôn chạy format trước khi báo xong.
- Quy ước đặt tên lệnh điều khiển: `dev.<domain>.<verb>_<noun>` (prefix `dev.` — TODO(team): đổi thành tên miền sản phẩm nếu muốn).
- TODO(team): nơi chạy test/CI (vd GitHub Actions: build idf.py + pytest + flutter test).
