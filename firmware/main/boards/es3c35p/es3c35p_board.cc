// Board ES3C35P: ST77922 QSPI 320x480 + LVGL + backlight.
// Cấu hình QSPI + chuỗi init LẤY NGUYÊN từ BSP chính chủ LCDWIKI (bsp_display.c) — đã kiểm chứng chạy.
#include "wifi_board.h"
#include "backlight.h"
#include "display/lcd_display.h"
#include "config.h"

#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_st77922.h>
#include <esp_lvgl_port.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>

#include "board.h"
#include "display/display.h"

#define TAG "es3c35p"
#define LCD_HOST SPI2_HOST

// Chuỗi init ST77922 320x480 — copy nguyên từ LCDWIKI bsp_display.c (panel-specific).
static const st77922_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xF1, (uint8_t[]){0x00}, 1, 0},
    {0x60, (uint8_t[]){0x00, 0x00, 0x00}, 3, 0},
    {0x65, (uint8_t[]){0x80}, 1, 0},
    {0x79, (uint8_t[]){0x06}, 1, 0},
    {0x7B, (uint8_t[]){0x00, 0x08, 0x08}, 3, 0},
    {0x80, (uint8_t[]){0x55, 0x62, 0x2F, 0x17, 0xF0, 0x52, 0x70, 0xD2, 0x52, 0x62, 0xEA}, 11, 0},
    {0x81, (uint8_t[]){0x26, 0x52, 0x72, 0x27}, 4, 0},
    {0x84, (uint8_t[]){0x92, 0x25}, 2, 0},
    {0x87, (uint8_t[]){0x10, 0x10, 0x58, 0x00, 0x02, 0x3A}, 6, 0},
    {0x88, (uint8_t[]){0x00, 0x00, 0x2C, 0x10, 0x04, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x06}, 15, 0},
    {0x89, (uint8_t[]){0x00, 0x00, 0x00}, 3, 0},
    {0x8A, (uint8_t[]){0x13, 0x00, 0x2C, 0x00, 0x00, 0x2C, 0x10, 0x10, 0x00, 0x3E, 0x19}, 11, 0},
    {0x8B, (uint8_t[]){0x15, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x97, 0x8E}, 9, 0},
    {0x8C, (uint8_t[]){0x1D, 0xB1, 0xB1, 0x44, 0x96, 0x2C, 0x10, 0x50, 0x0F, 0x01, 0xC5, 0x12, 0x09}, 13, 0},
    {0x8D, (uint8_t[]){0x0C}, 1, 0},
    {0x8E, (uint8_t[]){0x33, 0x01, 0x0C, 0x13, 0x01, 0x01}, 6, 0},
    {0xB3, (uint8_t[]){0x00, 0x30}, 2, 0},
    {0xF1, (uint8_t[]){0x00}, 1, 0},
    {0x71, (uint8_t[]){0xD0}, 1, 0},
    {0x66, (uint8_t[]){0x02, 0x3F}, 2, 0},
    {0xBE, (uint8_t[]){0x26, 0x00, 0x9D}, 3, 0},
    {0x70, (uint8_t[]){0x01, 0xA0, 0x11, 0x40, 0xE0, 0x00, 0x11, 0x69, 0x11, 0x00, 0x00, 0x1A}, 12, 0},
    {0x90, (uint8_t[]){0x04, 0x04, 0x55, 0x74, 0x00, 0x40, 0x43, 0x27, 0x27}, 9, 0},
    {0x91, (uint8_t[]){0x04, 0x04, 0x55, 0x75, 0x00, 0x40, 0x42, 0x27, 0x27}, 9, 0},
    {0x92, (uint8_t[]){0x04, 0x44, 0x55, 0xC0, 0x06, 0x00, 0x07, 0x05, 0x90, 0x27}, 10, 0},
    {0x93, (uint8_t[]){0x04, 0x43, 0x11, 0x00, 0x00, 0x00, 0x00, 0x05, 0x90, 0x27}, 10, 0},
    {0x94, (uint8_t[]){0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 6, 0},
    {0x95, (uint8_t[]){0x96, 0x16, 0x00, 0x00, 0xFF}, 5, 0},
    {0x96, (uint8_t[]){0x44, 0x53, 0x03, 0x12, 0x23, 0x24, 0x06, 0x05, 0x94, 0x27, 0x00, 0x44}, 12, 0},
    {0x97, (uint8_t[]){0x44, 0x53, 0x47, 0x56, 0x20, 0x20, 0x02, 0x01, 0x94, 0x27, 0x00, 0x44}, 12, 0},
    {0xBA, (uint8_t[]){0x55, 0x94, 0x2D, 0x94, 0x27}, 5, 0},
    {0x9A, (uint8_t[]){0x40, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00}, 7, 0},
    {0x9B, (uint8_t[]){0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00}, 7, 0},
    {0x9C, (uint8_t[]){0x5C, 0x12, 0x00, 0x00, 0x10, 0x12, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00}, 13, 0},
    {0x9D, (uint8_t[]){0x8A, 0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01}, 8, 0},
    {0x9E, (uint8_t[]){0x51, 0x00, 0x00, 0x00, 0x80, 0x1E, 0x01}, 7, 0},
    {0xB4, (uint8_t[]){0x1D, 0x1C, 0x1E, 0x0B, 0x14, 0x02, 0x13, 0x09, 0x1E, 0x00, 0x1E, 0x10}, 12, 0},
    {0xB5, (uint8_t[]){0x1D, 0x1C, 0x1E, 0x0A, 0x15, 0x03, 0x11, 0x08, 0x1E, 0x01, 0x1E, 0x12}, 12, 0},
    {0xB6, (uint8_t[]){0x77, 0x77, 0x00, 0x0A, 0xFF, 0x0A, 0xFF}, 7, 0},
    {0x86, (uint8_t[]){0xCD, 0x04, 0xB1, 0x02, 0x58, 0x12, 0x58, 0x0C, 0x13, 0x01, 0xA5, 0x00, 0xA5, 0xA5}, 14, 0},
    {0xB7, (uint8_t[]){0x07, 0x0A, 0x0E, 0x06, 0x05, 0x03, 0x2B, 0x03, 0x03, 0x42, 0x07, 0x10, 0x10, 0x2E, 0x3F, 0x0D}, 16, 0},
    {0xB8, (uint8_t[]){0x07, 0x0A, 0x0D, 0x05, 0x05, 0x02, 0x2B, 0x02, 0x03, 0x42, 0x06, 0x10, 0x0F, 0x2E, 0x3F, 0x0D}, 16, 0},
    {0xB9, (uint8_t[]){0x23, 0x23}, 2, 0},
    {0xBF, (uint8_t[]){0x10, 0x14, 0x14, 0x0B, 0x0B, 0x0B}, 6, 0},
    {0xF2, (uint8_t[]){0x00}, 1, 0},
    {0x73, (uint8_t[]){0x04, 0xDA, 0x12, 0x54, 0x47}, 5, 0},
    {0x77, (uint8_t[]){0x6B, 0x5B, 0xFD, 0xC3, 0xC5}, 5, 0},
    {0x7A, (uint8_t[]){0x15, 0x27}, 2, 0},
    {0x7B, (uint8_t[]){0x04, 0x57}, 2, 0},
    {0x7E, (uint8_t[]){0x01, 0x0E}, 2, 0},
    {0xBF, (uint8_t[]){0x36}, 1, 0},
    {0xE3, (uint8_t[]){0x40, 0x40}, 2, 0},
    {0xF0, (uint8_t[]){0x00}, 1, 0},
    {0xD0, (uint8_t[]){0x00}, 1, 0},
    {0x2A, (uint8_t[]){0x00, 0x00, 0x01, 0x3F}, 4, 0},
    {0x2B, (uint8_t[]){0x00, 0x00, 0x01, 0xDF}, 4, 0},
    {0x21, (uint8_t[]){0x00}, 0, 0},
    {0x11, (uint8_t[]){0x00}, 0, 120},
    {0x29, (uint8_t[]){0x00}, 0, 0},
    {0x2C, (uint8_t[]){0x00}, 0, 0},
    {0x3A, (uint8_t[]){0x01}, 1, 0},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x35, (uint8_t[]){0x01}, 1, 20},
};

// Giữ nút BOOT (active-low, IO0) >= 5s -> xóa WiFi + reboot vào chế độ cấu hình.
#define BOOT_HOLD_MS 5000
static void boot_button_task(void* arg) {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = 1ULL << BOOT_BUTTON_GPIO;
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&cfg);
    int held = 0;
    for (;;) {
        if (gpio_get_level(BOOT_BUTTON_GPIO) == 0) {  // đang nhấn
            held += 100;
            if (held == 3000) {  // giữ 3s: nhắc trên màn
                auto d = Board::GetInstance().GetDisplay();
                if (d) d->SetStatus("GIU DE CAU HINH WIFI");
            }
            if (held >= BOOT_HOLD_MS) {
                ESP_LOGW(TAG, "Giu BOOT >=5s -> vao che do cau hinh WiFi (giu WiFi cu)");
                auto d = Board::GetInstance().GetDisplay();
                if (d) d->SetStatus("WIFI SETUP");
                vTaskDelay(pdMS_TO_TICKS(600));
                WifiBoard::EnterConfigMode();  // đặt cờ + reboot, KHÔNG xóa WiFi cũ
            }
        } else {
            held = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ===== Touch ST77922 TDDI (I2C 0x55) — protocol theo BSP LCDWIKI =====
#define TOUCH_ADDR 0x55
static i2c_master_dev_handle_t s_tp_dev = nullptr;

static esp_err_t tp_read_reg(uint16_t reg, uint8_t* buf, size_t len) {
    uint8_t w[2] = {(uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF)};
    return i2c_master_transmit_receive(s_tp_dev, w, 2, buf, len, pdMS_TO_TICKS(50));
}

// LVGL đọc touch: reg 0x0010 bit3 = có chạm; 0x0014 = điểm 0 (7 byte).
static void touch_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    (void)indev;
    static bool was_pressed = false;
    bool pressed = false;
    int x = 0, y = 0;
    if (s_tp_dev) {
        uint8_t info = 0, pd[7] = {0};
        if (tp_read_reg(0x0010, &info, 1) == ESP_OK && (info & 0x08) &&
            tp_read_reg(0x0014, pd, 7) == ESP_OK && (pd[0] & 0x80)) {
            x = ((pd[0] & 0x3F) << 8) | pd[1];
            y = ((pd[2] & 0x3F) << 8) | pd[3];
            if (x < 0) x = 0;
            if (x >= DISPLAY_WIDTH) x = DISPLAY_WIDTH - 1;
            if (y < 0) y = 0;
            if (y >= DISPLAY_HEIGHT) y = DISPLAY_HEIGHT - 1;
            pressed = true;
        }
    }
    if (pressed) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
        if (!was_pressed) ESP_LOGI(TAG, "touch @%d,%d", x, y);  // log lúc chạm xuống
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    was_pressed = pressed;
}

class MakerSgBoard : public WifiBoard {
private:
    LcdDisplay* display_ = nullptr;
    Backlight* backlight_ = nullptr;

    void InitializeSpi() {
        ESP_LOGI(TAG, "Init QSPI bus (CLK=%d D0=%d D1=%d D2=%d D3=%d)", DISPLAY_CLK_PIN,
                 DISPLAY_DATA0_PIN, DISPLAY_DATA1_PIN, DISPLAY_DATA2_PIN, DISPLAY_DATA3_PIN);
        spi_bus_config_t buscfg = {};
        buscfg.sclk_io_num = DISPLAY_CLK_PIN;
        buscfg.data0_io_num = DISPLAY_DATA0_PIN;
        buscfg.data1_io_num = DISPLAY_DATA1_PIN;
        buscfg.data2_io_num = DISPLAY_DATA2_PIN;
        buscfg.data3_io_num = DISPLAY_DATA3_PIN;
        buscfg.max_transfer_sz = DISPLAY_WIDTH * 80 * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    }

    void InitializeDisplay() {
        ESP_LOGI(TAG, "Init ST77922 panel (pclk=%d MHz)", DISPLAY_PCLK_HZ / 1000000);
        esp_lcd_panel_io_handle_t io = nullptr;
        esp_lcd_panel_io_spi_config_t io_cfg = {};
        io_cfg.cs_gpio_num = DISPLAY_CS_PIN;
        io_cfg.dc_gpio_num = -1;
        io_cfg.spi_mode = 0;
        io_cfg.pclk_hz = DISPLAY_PCLK_HZ;
        io_cfg.trans_queue_depth = 10;
        io_cfg.lcd_cmd_bits = 32;
        io_cfg.lcd_param_bits = 8;
        io_cfg.flags.quad_mode = true;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_cfg, &io));

        st77922_vendor_config_t vendor = {};
        vendor.init_cmds = lcd_init_cmds;
        vendor.init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]);
        vendor.flags.use_qspi_interface = 1;

        esp_lcd_panel_dev_config_t panel_cfg = {};
        panel_cfg.reset_gpio_num = -1;  // reset chung EN/CHIP_PU
        panel_cfg.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_cfg.bits_per_pixel = 16;
        panel_cfg.vendor_config = &vendor;

        esp_lcd_panel_handle_t panel = nullptr;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st77922(io, &panel_cfg, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        // KHÔNG gọi invert_color (init đã có 0x21 INVON) — theo BSP LCDWIKI.
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

        display_ = new SpiLcdDisplay(io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_OFFSET_X,
                                     DISPLAY_OFFSET_Y, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y,
                                     DISPLAY_SWAP_XY);
    }

    void InitializeBacklight() {
        backlight_ = new PwmBacklight(DISPLAY_BACKLIGHT_PIN, DISPLAY_BACKLIGHT_OUTPUT_INVERT);
        backlight_->SetBrightness(80);
    }

    void InitializeTouch() {
        i2c_master_bus_config_t bus_cfg = {};
        bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
        bus_cfg.i2c_port = I2C_NUM_0;
        bus_cfg.sda_io_num = TOUCH_I2C_SDA_PIN;
        bus_cfg.scl_io_num = TOUCH_I2C_SCL_PIN;
        bus_cfg.glitch_ignore_cnt = 7;
        bus_cfg.flags.enable_internal_pullup = true;
        i2c_master_bus_handle_t bus = nullptr;
        if (i2c_new_master_bus(&bus_cfg, &bus) != ESP_OK) {
            ESP_LOGE(TAG, "Touch I2C bus fail");
            return;
        }
        // GPIO RST/INT y HỆT thư viện Arduino chính chủ (ST77922_Touch.cpp init()):
        // RST=output set HIGH trước; INT=INPUT thường (không pull). Rồi reset: low 100ms -> high 100ms.
        gpio_config_t rst_cfg = {};
        rst_cfg.pin_bit_mask = 1ULL << TOUCH_RST_PIN;
        rst_cfg.mode = GPIO_MODE_OUTPUT;
        gpio_config(&rst_cfg);
        gpio_set_level(TOUCH_RST_PIN, 1);
        gpio_config_t int_cfg = {};
        int_cfg.pin_bit_mask = 1ULL << TOUCH_INT_PIN;
        int_cfg.mode = GPIO_MODE_INPUT;
        gpio_config(&int_cfg);

        gpio_set_level(TOUCH_RST_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        gpio_set_level(TOUCH_RST_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(100));  // chờ chip khởi động xong

        // *** MẤU CHỐT: touch chạy 100kHz (thư viện chính chủ I2C_SPEED=100000), KHÔNG phải 400kHz. ***
        i2c_device_config_t dev = {};
        dev.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev.device_address = TOUCH_ADDR;
        dev.scl_speed_hz = 100000;
        if (i2c_master_bus_add_device(bus, &dev, &s_tp_dev) != ESP_OK) {
            ESP_LOGE(TAG, "Touch add dev fail");
            s_tp_dev = nullptr;
            return;
        }

        // Chờ chip sẵn sàng: đọc STATUS(0x0001) tới khi nibble thấp = 0 (như init() Arduino), có timeout.
        // Đây cũng là phép thử giao tiếp: nếu đọc luôn lỗi -> coi như không có touch, KHÔNG đăng ký indev
        // (tránh spam lỗi I2C mỗi frame).
        esp_log_level_set("i2c.master", ESP_LOG_NONE);
        bool ok = false;
        for (int i = 0; i < 50; i++) {  // tối đa ~1s
            uint8_t st = 0xFF;
            if (tp_read_reg(0x0001, &st, 1) == ESP_OK) {
                if ((st & 0x0F) == 0) {
                    ok = true;
                    break;
                }
            }
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        esp_log_level_set("i2c.master", ESP_LOG_WARN);
        if (!ok) {
            ESP_LOGW(TAG, "Touch @0x%02X khong phan hoi (100kHz) -> tat cam ung", TOUCH_ADDR);
            i2c_master_bus_rm_device(s_tp_dev);
            s_tp_dev = nullptr;
            return;
        }
        uint8_t maxp = 0;
        tp_read_reg(0x0009, &maxp, 1);
        ESP_LOGI(TAG, "Touch ready @0x%02X (100kHz, max pts=%d)", TOUCH_ADDR, maxp);

        if (lvgl_port_lock(0)) {
            lv_indev_t* indev = lv_indev_create();
            lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
            lv_indev_set_read_cb(indev, touch_read_cb);
            lvgl_port_unlock();
        }
    }

public:
    MakerSgBoard() {
        InitializeBacklight();
        InitializeSpi();
        InitializeDisplay();
        InitializeTouch();
        xTaskCreate(boot_button_task, "boot_btn", 3072, nullptr, 3, nullptr);
    }

    Display* GetDisplay() override { return display_; }
    Backlight* GetBacklight() override { return backlight_; }
    std::string GetBoardType() override { return "es3c35p"; }
};

DECLARE_BOARD(MakerSgBoard);
