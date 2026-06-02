#pragma once
// Pin-map ES3C35P (đầy đủ: ../../iot-fw-module/references/board-es3c35p.md).
// Phase 1 chỉ cần LED trạng thái + nút BOOT; màn/audio/touch để phase sau.

#define PIN_STATUS_LED   40   // LED RGB địa chỉ hóa (WS2812-like) — báo trạng thái
#define PIN_BOOT_BUTTON   0   // nút BOOT
#define PIN_LCD_BACKLIGHT 41  // backlight — PWM (LEDC) chỉnh độ sáng; phải có mức cao mới sáng

// LCD ST77922 QSPI (bảng 4.2 spec). Reset dùng chung CHIP_PU (không pin reset riêng).
#define PIN_LCD_CS   10
#define PIN_LCD_CLK  12
#define PIN_LCD_D0    9
#define PIN_LCD_D1   11
#define PIN_LCD_D2   13
#define PIN_LCD_D3   14
