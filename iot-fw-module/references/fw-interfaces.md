# Firmware interfaces — ESP-IDF (TODO(team) trỏ file thật khi code tồn tại)

Tham chiếu nhanh interface mỗi loại module. Mọi recipe trong SKILL.md xoay quanh các interface dưới đây. Framework: **ESP-IDF** (C/C++).

> Phần cứng (pin-map, ngoại vi sẵn có) đã có ở [board-es3c35p.md](board-es3c35p.md). File này tập trung vào **interface phần mềm** — điền grep pattern tới code thật khi có.
> Ngoại vi của board cần driver: Display (ST77922 QSPI), Touch (I2C 0x38), Audio codec (I2S+mic+PA), LED RGB (IO40), SD (SDIO), Battery ADC (IO8).

## 1. Driver / peripheral base

- Cách tiếp cận ESP-IDF: mỗi driver là một component bọc HAL/driver có sẵn:
  - Display → `esp_lcd` (panel ST77922 QSPI, `esp_lcd_new_panel_io_*` + custom panel driver).
  - Touch → `i2c_master` + `esp_lcd_touch` (bus chung IO38/IO39, addr 0x38).
  - LED RGB → component `led_strip` (RMT, kiểu WS2812, IO40).
  - Audio → `i2s_std` + driver codec qua I2C (PA enable IO1).
  - SD → `sdmmc` (SDIO 4-bit), Battery → `adc_oneshot` (IO8).
- Hàm/struct bắt buộc: TODO(team) — `grep` base/handle pattern khi code thật tồn tại.
- File mẫu chuẩn: TODO(team) — trỏ driver gần nhất với case đang làm.

## 2. Command handler / registry

- Nơi đăng ký: TODO(team) — `grep` hàm register command (vd nơi parse `name` của lệnh từ cloud rồi dispatch).
- API đăng ký + khai báo argument: parse `args` bằng `cJSON` (`cJSON_GetObjectItem`, `cJSON_GetStringValue`, `cJSON_GetNumberValue`); kiểm required vs default + range thủ công.
- Quy ước tên lệnh: `dev.<domain>.<verb>_<noun>` — chuỗi literal khớp tuyệt đối với cloud.
- File mẫu: TODO(team).

## 3. Message / telemetry schema

- Struct message lên cloud: gói JSON bằng `cJSON_CreateObject` theo envelope `{"type", "data"}`. TODO(team): liệt kê `type` thật.
- Định dạng dây: JSON text qua transport (mục 4). TODO(team) xác nhận nếu dùng binary.

## 4. Protocol / comms base

- Transport tới cloud: gợi ý WebSocket client (`esp_websocket_client`) — khớp với cloud FastAPI WebSocket. TODO(team) xác nhận nếu MQTT (`esp-mqtt`).
- Lifecycle + callback: đăng ký event handler (`esp_event`) cho CONNECTED/DATA/DISCONNECTED; gửi qua một lớp chung, không mở socket rải rác.
- File mẫu: TODO(team).

## Helper hữu ích

- Truy cập phần cứng: TODO(team) — pattern board/bsp singleton hoặc handle toàn cục (vd getter trả `esp_lcd_panel_handle_t`, `led_strip_handle_t`).
- Bộ nhớ: buffer lớn (>4KB) xin từ PSRAM — `heap_caps_malloc(n, MALLOC_CAP_SPIRAM)` fallback `MALLOC_CAP_8BIT`.
