// LCD UI tối ưu RAM cho ES3C35P (ST77922 QSPI 320x480).
// Chiến lược RAM: KHÔNG LVGL, KHÔNG framebuffer 320x480 (300KB). Chỉ 1 "dải vẽ" rộng 320 x cao 32
// = 20KB trong SRAM nội DMA, tái dùng cho mọi thao tác (clear theo dải, vẽ 1 dòng text, fill bar).
#include "display.h"

#include <string.h>

#include "board.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st77922.h"  // managed component espressif/esp_lcd_st77922
#include "esp_log.h"
#include "font8x8.h"

static const char *TAG = "lcd";

#define DISP_W 320
#define DISP_H 480
#define BAND_H 32                 // cao tối đa 1 dải (đủ text scale 4 = 32px)
#define LCD_HOST SPI2_HOST
#define SWAP_BYTES 1              // panel RGB565 thường big-endian; đổi 0 nếu màu sai

#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
#define C_BG RGB565(15, 23, 42)    // slate-900
#define C_FG RGB565(241, 245, 249) // slate-100
#define C_ACCENT RGB565(26, 111, 245)
#define C_OK RGB565(16, 185, 129)
#define C_WARN RGB565(245, 158, 11)

static esp_lcd_panel_handle_t s_panel;
static uint16_t *s_band;  // DISP_W * BAND_H, DMA nội — tái dùng

// ---- Backlight PWM (LEDC) ----
#define BL_MODE LEDC_LOW_SPEED_MODE
#define BL_TIMER LEDC_TIMER_0
#define BL_CHANNEL LEDC_CHANNEL_0
#define BL_RES LEDC_TIMER_10_BIT  // duty 0..1023
#define BL_FREQ 5000              // 5 kHz (không ù, ngoài ngưỡng nghe)
#define BL_DEFAULT 80             // độ sáng mặc định %

static void backlight_init(void) {
    ledc_timer_config_t t = {
        .speed_mode = BL_MODE,
        .timer_num = BL_TIMER,
        .duty_resolution = BL_RES,
        .freq_hz = BL_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&t);
    ledc_channel_config_t c = {
        .gpio_num = PIN_LCD_BACKLIGHT,
        .speed_mode = BL_MODE,
        .channel = BL_CHANNEL,
        .timer_sel = BL_TIMER,
        .duty = 0,  // tắt lúc đầu, tránh chớp màn nhiễu trước khi panel sẵn sàng
        .hpoint = 0,
    };
    ledc_channel_config(&c);
}

void display_set_backlight(int pct) {
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    uint32_t duty = ((1u << 10) - 1) * (uint32_t)pct / 100;
    ledc_set_duty(BL_MODE, BL_CHANNEL, duty);
    ledc_update_duty(BL_MODE, BL_CHANNEL);
}

static inline uint16_t enc(uint16_t c) {
#if SWAP_BYTES
    return (uint16_t)((c >> 8) | (c << 8));
#else
    return c;
#endif
}

// Fill chữ nhật x,y,w,h màu solid — đi theo dải BAND_H, dùng s_band (đóng gói rộng w liên tục).
static void fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (!s_panel || !s_band || w <= 0 || h <= 0) return;
    if (w > DISP_W) w = DISP_W;
    uint16_t e = enc(color);
    for (int done = 0; done < h; done += BAND_H) {
        int rows = (h - done) < BAND_H ? (h - done) : BAND_H;
        for (int i = 0; i < w * rows; i++) s_band[i] = e;
        esp_lcd_panel_draw_bitmap(s_panel, x, y + done, x + w, y + done + rows, s_band);
    }
}

static const uint8_t *glyph(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c < 0x20 || c > 0x5F) c = 0x20;
    return FONT8X8[c - 0x20];
}

// Vẽ 1 dòng text căn giữa tại y. Dải rộng DISP_W, cao h=8*scale (<=BAND_H). 1 lần blit.
static void draw_text(int y, const char *s, int scale, uint16_t fg, uint16_t bg, bool center) {
    if (!s_panel || !s_band) return;
    int h = 8 * scale;
    if (h > BAND_H) {
        scale = BAND_H / 8;
        h = 8 * scale;
    }
    uint16_t ebg = enc(bg), efg = enc(fg);
    for (int i = 0; i < DISP_W * h; i++) s_band[i] = ebg;  // clear dải

    int len = (int)strlen(s);
    int cw = 8 * scale;
    int x0 = center ? (DISP_W - len * cw) / 2 : 8;
    if (x0 < 0) x0 = 0;

    for (int i = 0; i < len; i++) {
        const uint8_t *g = glyph(s[i]);
        int cx = x0 + i * cw;
        for (int row = 0; row < 8; row++) {
            uint8_t bits = g[row];
            for (int col = 0; col < 8; col++) {
                if (!(bits & (0x80 >> col))) continue;
                for (int sy = 0; sy < scale; sy++) {
                    int py = row * scale + sy;
                    for (int sx = 0; sx < scale; sx++) {
                        int px = cx + col * scale + sx;
                        if (px >= 0 && px < DISP_W && py >= 0 && py < h) s_band[py * DISP_W + px] = efg;
                    }
                }
            }
        }
    }
    esp_lcd_panel_draw_bitmap(s_panel, 0, y, DISP_W, y + h, s_band);
}

static void clear_screen(void) { fill_rect(0, 0, DISP_W, DISP_H, C_BG); }

static void brand(void) { draw_text(28, "MAKERSG", 2, C_ACCENT, C_BG, true); }

bool display_init(void) {
    // 1. Backlight (IO41) — PWM LEDC, tắt lúc đầu; bật mượt (fade-in) sau khi panel sẵn sàng.
    backlight_init();

    // 2. QSPI bus + panel ST77922. (Macro từ esp_lcd_st77922 — xác nhận theo ví dụ/phiên bản component.)
    spi_bus_config_t buscfg = ST77922_PANEL_BUS_QSPI_CONFIG(
        PIN_LCD_CLK, PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3,
        DISP_W * BAND_H * (int)sizeof(uint16_t));
    if (spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO) != ESP_OK) {
        ESP_LOGE(TAG, "spi bus init fail");
        return false;
    }

    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = ST77922_PANEL_IO_QSPI_CONFIG(PIN_LCD_CS, NULL, NULL);
    if (esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &io) != ESP_OK) {
        ESP_LOGE(TAG, "panel io fail");
        return false;
    }

    st77922_vendor_config_t vendor = {.flags = {.use_qspi_interface = 1}};
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = -1,  // reset chung CHIP_PU, không toggle riêng được
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config = &vendor,
    };
    if (esp_lcd_new_panel_st77922(io, &panel_cfg, &s_panel) != ESP_OK) {
        ESP_LOGE(TAG, "panel st77922 fail");
        s_panel = NULL;
        return false;
    }
    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    esp_lcd_panel_disp_on_off(s_panel, true);

    // 3. Dải vẽ 20KB trong SRAM nội DMA (tối ưu RAM — không full framebuffer).
    s_band = heap_caps_malloc(DISP_W * BAND_H * sizeof(uint16_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!s_band) {
        ESP_LOGE(TAG, "band alloc fail");
        s_panel = NULL;
        return false;
    }
    ESP_LOGI(TAG, "LCD ready (band %d KB SRAM)", DISP_W * BAND_H * 2 / 1024);
    clear_screen();
    brand();
    // Fade-in backlight tới mặc định (tránh chớp màn lúc bật).
    for (int p = 0; p <= BL_DEFAULT; p += 8) {
        display_set_backlight(p);
        vTaskDelay(pdMS_TO_TICKS(15));
    }
    display_set_backlight(BL_DEFAULT);
    return true;
}

void display_status(const char *title, const char *line1, const char *line2) {
    if (!s_panel) return;
    clear_screen();
    brand();
    if (title) draw_text(170, title, 3, C_FG, C_BG, true);
    if (line1) draw_text(250, line1, 2, C_FG, C_BG, true);
    if (line2) draw_text(290, line2, 2, C_WARN, C_BG, true);
}

void display_show_code(const char *code) {
    if (!s_panel) return;
    clear_screen();
    brand();
    draw_text(150, "DEVICE CODE", 2, C_WARN, C_BG, true);
    draw_text(205, code ? code : "------", 4, C_FG, C_BG, true);
    draw_text(330, "ENTER ON SERVER", 2, C_ACCENT, C_BG, true);
}

#define BAR_X 30
#define BAR_Y 270
#define BAR_W 260
#define BAR_H 28

void display_ota_begin(void) {
    if (!s_panel) return;
    clear_screen();
    brand();
    draw_text(160, "OTA UPDATE", 3, C_FG, C_BG, true);
    // Khung thanh tiến trình
    fill_rect(BAR_X, BAR_Y, BAR_W, BAR_H, C_FG);
    fill_rect(BAR_X + 2, BAR_Y + 2, BAR_W - 4, BAR_H - 4, C_BG);
    display_progress(0);
}

void display_progress(int pct) {
    if (!s_panel) return;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    int inner = (BAR_W - 4) * pct / 100;
    if (inner > 0) fill_rect(BAR_X + 2, BAR_Y + 2, inner, BAR_H - 4, C_OK);

    char buf[8];
    int n = pct, i = 0;
    char tmp[4];
    if (n == 0) tmp[i++] = '0';
    while (n > 0 && i < 3) {
        tmp[i++] = (char)('0' + n % 10);
        n /= 10;
    }
    int j = 0;
    while (i > 0) buf[j++] = tmp[--i];
    buf[j++] = '%';
    buf[j] = 0;
    draw_text(BAR_Y + BAR_H + 16, buf, 3, C_FG, C_BG, true);
}
