#pragma once
#include <stdbool.h>

// UI trạng thái trên LCD ST77922 (320x480). Tối ưu RAM: không LVGL, không full framebuffer,
// chỉ 1 dải vẽ ~20KB trong SRAM nội (DMA). Mọi hàm an toàn nếu LCD chưa init (no-op → chạy headless).

bool display_init(void); // true nếu LCD lên

void display_set_backlight(int pct); // độ sáng 0..100 (PWM LEDC trên IO41)

void display_status(const char *title, const char *line1, const char *line2);
void display_show_code(const char *code); // màn "DEVICE CODE" + mã to
void display_ota_begin(void);             // màn OTA + khung thanh tiến trình (gọi 1 lần)
void display_progress(int pct);           // cập nhật thanh + % (gọi trong vòng OTA)
