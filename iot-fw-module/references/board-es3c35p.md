# Board reference — LCDWIKI ES3C35P (3.5" IPS ESP32-S3)

Nguồn: LCDWIKI "3.5 inch IPS ESP32-S3 Specification" V1.0 (2025-06-14), SKU **ES3C35P**.
Đây là nguồn sự thật phần cứng cho mọi recipe firmware. Pin-map dưới đây quyết định cấu hình board.

## MCU & bộ nhớ

| Mục | Giá trị |
|---|---|
| Chip | ESP32-S3, Xtensa LX7 dual-core, 240 MHz |
| SRAM | 512 KB + 16 KB RTC SRAM |
| ROM | 384 KB (spec ghi "84KB" — lỗi đánh máy) |
| PSRAM | **8 MB OPI PSRAM nội** (dùng cho buffer lớn) |
| Flash | 16 MB SPI/QSPI ngoài |
| Vô tuyến | WiFi 2.4GHz b/g/n; BT 5.0 BR/EDR + BLE |
| Build target | `esp32s3` (cố định) |
| Nguồn | 5V qua Type-C; MCU 3.0–3.6V |

## Màn hình (LCD)

| Mục | Giá trị |
|---|---|
| Kích thước / phân giải | 3.5" IPS TFT, **320×480** (320×RGB×480) |
| Driver IC | **ST77922** |
| Giao tiếp | **QSPI** (nối thẳng ESP32-S3) — KHÔNG phải SPI thường |
| Màu | RGB565 (typ) / RGB888 (max) |
| Backlight | LED trắng, mức cao = bật, thấp = tắt, **PWM chỉnh độ sáng được** |

## Pin-map (theo bảng 4.2 của spec)

### LCD (QSPI)
| Pin | Chức năng |
|---|---|
| IO10 | LCD CS (chip select, active low) |
| IO12 | LCD QSPI clock |
| IO9, IO11, IO13, IO14 | LCD QSPI data D0–D3 (xác nhận thứ tự D0→D3 từ code mẫu) |
| CHIP_PU | LCD reset (**dùng chung reset với MCU**, active low) |
| IO41 | Backlight control (PWM) |

### Cảm ứng điện dung (I2C)
| Pin | Chức năng |
|---|---|
| IO38 | Touch I2C SDA (**chung bus** với audio codec + I2C mở rộng) |
| IO39 | Touch I2C SCL (chung bus) |
| IO48 | Touch reset (active low) |
| IO47 | Touch interrupt (low khi chạm) |

Địa chỉ I2C touch: **0x38** (0x38 thường là họ FocalTech FT6x36/FT5x06 — xác nhận IC thật từ schematic).

### Audio (I2S + codec + PA + mic)
| Pin | Chức năng |
|---|---|
| IO1 | Power amplifier enable |
| IO17 | I2S MCLK |
| IO18 | I2S BCLK |
| IO15 | I2S DOUT (ra loa) |
| IO16 | I2S DIN (từ mic) |
| IO21 | I2S LRCK/WS (cao=phải, thấp=trái) |
| IO38 / IO39 | Audio codec I2C SDA/SCL (chung bus touch) |

Mic: MEMS hướng xuống. Loa ngoài: max **1.5W@8Ω hoặc 2W@4Ω**. Codec IC + địa chỉ I2C: spec không nêu — xác nhận từ schematic (board LCDWIKI loại này thường dùng ES8311, cần kiểm chứng).

### LED RGB
| Pin | Chức năng |
|---|---|
| IO40 | LED RGB **địa chỉ hóa** (1 dây, có IC điều khiển nội — kiểu WS2812) |

### Thẻ MicroSD (SDIO)
| Pin | Chức năng |
|---|---|
| IO5 | CLK |
| IO4 | CMD |
| IO6 / IO7 / IO2 / IO3 | DATA0 / DATA1 / DATA2 / DATA3 |

### Pin & nút khác
| Pin | Chức năng |
|---|---|
| IO8 | ADC đọc điện áp pin |
| IO0 | Nút BOOT (giữ khi cấp nguồn → vào download mode) |
| CHIP_PU | Nút RESET (chung reset LCD) |
| IO19 / IO20 | USB D− / D+ |
| IO43 / IO44 | UART0 TX / RX (spec ghi cả hai là "receive" — lỗi spec; quy ước IO43=TX, IO44=RX, xác nhận từ code) |
| IO45, IO46 | GPIO trống cho mở rộng |
| IO26–IO37 | OPI PSRAM nội (**không dùng làm GPIO**) |
| IO27–IO32 | QSPI flash ngoài (dùng chung với PSRAM, **không dùng làm GPIO**) |

## Ràng buộc quan trọng cho firmware

- **Một bus I2C dùng chung** (IO38/IO39): touch + audio codec + mở rộng. Cùng một driver bus, nhiều device khác địa chỉ. Đừng tạo hai bus.
- **Reset dùng chung**: LCD reset và MCU reset cùng CHIP_PU — không có pin reset LCD riêng để toggle độc lập.
- **Display là QSPI** (không phải SPI 1-data) — chọn đúng driver ST77922 QSPI khi cấu hình.
- **Buffer lớn → PSRAM**: có 8MB OPI PSRAM; framebuffer/ảnh/audio nên `heap_caps_malloc(.., MALLOC_CAP_SPIRAM)`, fallback `MALLOC_CAP_8BIT`.
- **Backlight phải kéo cao** mới sáng (FAQ #1 của spec) — code init bắt buộc bật IO41.
- **GPIO trống chỉ còn IO45, IO46** (cộng GND/VCC) cho peripheral ngoài.

## Bản đồ ngoại vi → module firmware

| Ngoại vi | Module firmware tương ứng (recipe trong SKILL.md) |
|---|---|
| Màn ST77922 QSPI 320×480 + backlight | Display |
| Touch điện dung I2C 0x38 + INT IO47 | Driver touch (input) |
| Codec I2S + mic + PA (IO1) | Audio codec |
| LED RGB IO40 (WS2812-like) | Driver LED/indicator |
| Thẻ SD SDIO | Storage |
| ADC pin IO8 | Battery monitor |

> Phần stack/build framework cụ thể (ESP-IDF vs Arduino/PlatformIO) và các tính năng dùng những ngoại vi này: TODO(team) điền sau.
