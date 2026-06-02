# firmware/ — ES3C35P Phase 1 (WiFi provisioning + OTA + WebSocket)

Project ESP-IDF cho lớp kết nối lõi của board LCDWIKI ES3C35P (ESP32-S3).
Hợp đồng: [../PHASE1.md](../PHASE1.md). Pin-map: [../iot-fw-module/references/board-es3c35p.md](../iot-fw-module/references/board-es3c35p.md).

> Phase 1 chỉ làm: provisioning WiFi (SoftAP) → OTA check/update → kết nối WebSocket + hello.
> Màn/audio/touch/LED-driver để phase sau (đã có pin trong `board.h`).

## Giao diện LCD (ST77922 QSPI) — tối ưu RAM

UI trạng thái hiển thị suốt luồng, đặc biệt **bước OTA**:
- `MAKERSG / BOOTING` → `WIFI SETUP` (SSID AP + 192.168.4.1) → `WIFI CONNECTING` → `OTA CHECKING`
- **Chưa đăng ký** → màn **DEVICE CODE** + mã 6 số to (`display_show_code`)
- **Có firmware mới** → màn **OTA UPDATE** + **thanh tiến trình %** realtime (`display_ota_begin`/`display_progress`, dùng `esp_https_ota` granular)
- WS connected → `ONLINE / DEVICE READY`; mất kết nối → `OFFLINE / RECONNECTING`

**Backlight:** PWM qua LEDC (IO41, 5kHz, 10-bit) — `display_set_backlight(0..100)`. Mặc định 80%, **fade-in** mượt lúc khởi động (tắt → 80%) để tránh chớp màn nhiễu trước khi panel sẵn sàng.

**Tối ưu RAM (yêu cầu):** KHÔNG LVGL, KHÔNG full framebuffer 320×480 (~300KB). Chỉ **một dải vẽ 320×32×2 = 20KB** trong SRAM nội DMA, tái dùng cho clear/text/bar (`display.c`). Font 8×8 nằm trong flash (`font8x8.h`), không tốn RAM. Chữ trên màn dùng HOA+số+ký hiệu (tự chuyển HOA).

> Driver màn: managed component `espressif/esp_lcd_st77922` (khai trong `idf_component.yml`). Macro init QSPI (`ST77922_PANEL_BUS_QSPI_CONFIG`…) **cần xác nhận theo ví dụ/phiên bản component** khi build. Nếu màu sai → đổi `SWAP_BYTES` trong `display.c`.

## Luồng (main.c)

```
app_main → nvs init → esp_wifi init
   ├─ chưa có WiFi  → SoftAP "Xiaozhi-Setup" + portal http://192.168.4.1 → lưu NVS → restart
   ├─ có WiFi       → STA connect
   ├─ OTA check     → POST /api/v1/ota (device-id = MAC)
   │     ├─ chưa đăng ký → log mã activation, thử lại tới khi đăng ký
   │     ├─ có firmware mới → esp_https_ota → restart
   │     └─ trả websocket {url, token}
   └─ WS connect    → header device-id + Bearer token → gửi hello
```

## Cấu hình trước khi build

Sửa [main/app_config.h](main/app_config.h):
- `OTA_URL` → trỏ về server cloud (IP LAN máy chạy `cloud/`), vd `http://192.168.1.100:8000/api/v1/ota`.
- `FW_APP_VERSION` → tăng mỗi khi build bản OTA mới.
- (tùy chọn) `PROV_AP_SSID/PASS`.

## Build & nạp

```bash
idf.py set-target esp32s3
idf.py menuconfig        # xác nhận PSRAM OPI 8MB, flash 16MB, partition table = partitions.csv
idf.py build flash monitor
```

Lần đầu chưa có WiFi → điện thoại/PC kết nối AP **Xiaozhi-Setup** → mở `http://192.168.4.1` → nhập SSID/mật khẩu → thiết bị restart và tự chạy luồng OTA + WS.

## Phụ thuộc

- Managed component `espressif/esp_websocket_client` (khai trong [main/idf_component.yml](main/idf_component.yml)) — `idf.py` tự tải.
- Còn lại là component IDF core (esp_wifi, esp_http_client, esp_https_ota, esp_http_server, json/cJSON).

## Lưu ý

- OTA Phase 1 chạy qua **HTTP** (LAN). Production nên HTTPS (`esp_https_ota` + cert) — TODO.
- Bảng phân vùng [partitions.csv](partitions.csv) có 2 slot app (ota_0/ota_1) để OTA luân phiên.
- Một số symbol `sdkconfig.defaults` (PSRAM) đổi theo phiên bản IDF — kiểm `menuconfig`.
- Chưa build/kiểm trên phần cứng trong môi trường này; cần `idf.py build` + board thật để xác nhận.
