#include "lcd_display.h"

#include <esp_lvgl_port.h>

#include "app_config.h"  // FW_APP_VERSION

#define TAG "LcdDisplay"

// Trampoline sự kiện nút: user_data = con trỏ tới std::function<void()> (bền vì sống trong LcdDisplay).
static void pomo_btn_cb(lv_event_t* e) {
    auto* fn = static_cast<std::function<void()>*>(lv_event_get_user_data(e));
    if (fn && *fn) (*fn)();
}

// Tạo nút bo góc + nhãn căn giữa. Trả nút; nếu out_lbl != null thì trả luôn con trỏ nhãn.
static lv_obj_t* make_button(lv_obj_t* parent, const char* text, uint32_t bg, const lv_font_t* font,
                             std::function<void()>* handler, lv_obj_t** out_lbl = nullptr) {
    lv_obj_t* btn = lv_button_create(parent);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg), 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    if (handler) lv_obj_add_event_cb(btn, pomo_btn_cb, LV_EVENT_CLICKED, handler);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    if (font) lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_center(lbl);
    if (out_lbl) *out_lbl = lbl;
    return btn;
}

LcdDisplay::LcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width,
                       int height)
    : panel_io_(panel_io), panel_(panel) {
    width_ = width;
    height_ = height;
}

LcdDisplay::~LcdDisplay() {
    if (display_ != nullptr) lv_display_delete(display_);
    if (panel_ != nullptr) esp_lcd_panel_del(panel_);
    if (panel_io_ != nullptr) esp_lcd_panel_io_del(panel_io_);
}

bool LcdDisplay::Lock(int timeout_ms) { return lvgl_port_lock(timeout_ms); }
void LcdDisplay::Unlock() { lvgl_port_unlock(); }

void LcdDisplay::SetupUI() {
    DisplayLockGuard lock(this);
    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);  // nền đen như brand
    lv_obj_set_style_text_color(screen, lv_color_hex(0xFFFFFF), 0);

    // Logo chữ: "MAKER SG" lớn + "WORKSHOP" giãn chữ, gom giữa màn.
    container_ = lv_obj_create(screen);
    lv_obj_remove_style_all(container_);
    lv_obj_set_size(container_, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_center(container_);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    title_label_ = lv_label_create(container_);
    lv_label_set_text(title_label_, "MAKER SG");
    lv_obj_set_style_text_color(title_label_, lv_color_hex(0xFFFFFF), 0);
#if LV_FONT_MONTSERRAT_48
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_48, 0);
#elif LV_FONT_MONTSERRAT_28
    lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_28, 0);
#endif

    lv_obj_t* sub = lv_label_create(container_);
    lv_label_set_text(sub, "WORKSHOP");
    lv_obj_set_style_text_color(sub, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_letter_space(sub, 6, 0);
    lv_obj_set_style_pad_top(sub, 6, 0);
#if LV_FONT_MONTSERRAT_28
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_28, 0);
#endif

    // Dòng trạng thái (BOOTING/WIFI/OTA/ONLINE...). FULL chiều rộng + căn giữa: khi đổi chữ,
    // vùng vẽ lại luôn là cả dải cố định -> xóa sạch chữ cũ, hết "bóng ma"/nhiễu khi text co lại.
    status_label_ = lv_label_create(screen);
    lv_label_set_text(status_label_, "");
    lv_obj_set_width(status_label_, LV_PCT(100));
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label_, lv_color_hex(0x808080), 0);
    lv_obj_align(status_label_, LV_ALIGN_BOTTOM_MID, 0, -12);

    // Version firmware ở góc TRÊN phải (tách khỏi vùng trạng thái động ở đáy).
    lv_obj_t* ver = lv_label_create(screen);
    lv_label_set_text(ver, "v" FW_APP_VERSION);
    lv_obj_set_style_text_color(ver, lv_color_hex(0x606060), 0);
    lv_obj_align(ver, LV_ALIGN_TOP_RIGHT, -10, 10);

    // Cường độ WiFi: 4 vạch sóng ở góc TRÊN trái (cao dần).
    lv_obj_t* wbox = lv_obj_create(screen);
    lv_obj_remove_style_all(wbox);
    lv_obj_clear_flag(wbox, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(wbox, 26, 18);
    lv_obj_align(wbox, LV_ALIGN_TOP_LEFT, 10, 10);
    const int bw = 4, gap = 2, hts[4] = {5, 9, 13, 17};
    for (int i = 0; i < 4; i++) {
        wifi_bars_[i] = lv_obj_create(wbox);
        lv_obj_remove_style_all(wifi_bars_[i]);
        lv_obj_clear_flag(wifi_bars_[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(wifi_bars_[i], bw, hts[i]);
        lv_obj_set_pos(wifi_bars_[i], i * (bw + gap), 17 - hts[i]);  // căn đáy
        lv_obj_set_style_radius(wifi_bars_[i], 1, 0);
        lv_obj_set_style_bg_opa(wifi_bars_[i], LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(wifi_bars_[i], lv_color_hex(0x303030), 0);
    }

    // Màn đồng hồ (ẩn ban đầu, hiện khi đã sync giờ — thay logo).
    // FULL chiều rộng + căn giữa text + Y CỐ ĐỊNH: khi giây đổi, vùng vẽ lại là cả dải ngang cố định
    // -> xóa sạch chữ số cũ, hết nhiễu/bóng ma (cùng cách đã fix status_label_).
    clock_time_ = lv_label_create(screen);
    lv_label_set_text(clock_time_, "--:--:--");
    lv_obj_set_width(clock_time_, LV_PCT(100));
    lv_obj_set_style_text_align(clock_time_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(clock_time_, lv_color_hex(0xFFFFFF), 0);
#if LV_FONT_MONTSERRAT_48
    lv_obj_set_style_text_font(clock_time_, &lv_font_montserrat_48, 0);
#endif
    lv_obj_align(clock_time_, LV_ALIGN_CENTER, 0, -16);
    lv_obj_add_flag(clock_time_, LV_OBJ_FLAG_HIDDEN);

    clock_date_ = lv_label_create(screen);
    lv_label_set_text(clock_date_, "");
    lv_obj_set_width(clock_date_, LV_PCT(100));
    lv_obj_set_style_text_align(clock_date_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(clock_date_, lv_color_hex(0x9CA3AF), 0);
#if LV_FONT_MONTSERRAT_28
    lv_obj_set_style_text_font(clock_date_, &lv_font_montserrat_28, 0);
#endif
    lv_obj_align(clock_date_, LV_ALIGN_CENTER, 0, 44);
    lv_obj_add_flag(clock_date_, LV_OBJ_FLAG_HIDDEN);

    // Nút mở Pomodoro (chỉ hiện khi đang ở màn đồng hồ).
    btn_pomo_ = make_button(screen, "POMODORO", 0x1F2937, nullptr, &on_pomo_enter_);
    lv_obj_set_size(btn_pomo_, 180, 46);
    lv_obj_align(btn_pomo_, LV_ALIGN_BOTTOM_MID, 0, -52);
    lv_obj_add_flag(btn_pomo_, LV_OBJ_FLAG_HIDDEN);

    SetupPomodoroUI(screen);
}

void LcdDisplay::SetupPomodoroUI(lv_obj_t* screen) {
    // Container phủ kín màn (đen, đục) — nằm trên cùng; bật/tắt = hiện/ẩn cả màn Pomodoro.
    pomo_cont_ = lv_obj_create(screen);
    lv_obj_remove_style_all(pomo_cont_);
    lv_obj_set_size(pomo_cont_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(pomo_cont_, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(pomo_cont_, LV_OPA_COVER, 0);
    lv_obj_clear_flag(pomo_cont_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(pomo_cont_, LV_OBJ_FLAG_HIDDEN);

    // Nhãn phase trên cùng.
    pomo_phase_ = lv_label_create(pomo_cont_);
    lv_label_set_text(pomo_phase_, "LAM VIEC");
    lv_obj_set_width(pomo_phase_, LV_PCT(100));
    lv_obj_set_style_text_align(pomo_phase_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(pomo_phase_, lv_color_hex(0x1A6FF5), 0);
#if LV_FONT_MONTSERRAT_28
    lv_obj_set_style_text_font(pomo_phase_, &lv_font_montserrat_28, 0);
#endif
    lv_obj_align(pomo_phase_, LV_ALIGN_TOP_MID, 0, 54);

    // Vòng tiến trình.
    pomo_arc_ = lv_arc_create(pomo_cont_);
    lv_obj_set_size(pomo_arc_, 240, 240);
    lv_obj_align(pomo_arc_, LV_ALIGN_CENTER, 0, -40);
    lv_arc_set_rotation(pomo_arc_, 270);       // bắt đầu từ đỉnh
    lv_arc_set_bg_angles(pomo_arc_, 0, 360);   // nền nguyên vòng
    lv_arc_set_range(pomo_arc_, 0, 100);
    lv_arc_set_value(pomo_arc_, 0);
    lv_obj_clear_flag(pomo_arc_, LV_OBJ_FLAG_CLICKABLE);  // không chặn chạm
    lv_obj_remove_style(pomo_arc_, nullptr, LV_PART_KNOB);  // bỏ núm kéo
    lv_obj_set_style_arc_width(pomo_arc_, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(pomo_arc_, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(pomo_arc_, lv_color_hex(0x263042), LV_PART_MAIN);
    lv_obj_set_style_arc_color(pomo_arc_, lv_color_hex(0x1A6FF5), LV_PART_INDICATOR);

    // Thời gian lớn ở giữa vòng.
    pomo_time_ = lv_label_create(pomo_cont_);
    lv_label_set_text(pomo_time_, "25:00");
    lv_obj_set_width(pomo_time_, 220);
    lv_obj_set_style_text_align(pomo_time_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(pomo_time_, lv_color_hex(0xFFFFFF), 0);
#if LV_FONT_MONTSERRAT_48
    lv_obj_set_style_text_font(pomo_time_, &lv_font_montserrat_48, 0);
#endif
    lv_obj_align(pomo_time_, LV_ALIGN_CENTER, 0, -50);

    pomo_session_ = lv_label_create(pomo_cont_);
    lv_label_set_text(pomo_session_, "Phien 1/4");
    lv_obj_set_width(pomo_session_, 220);
    lv_obj_set_style_text_align(pomo_session_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(pomo_session_, lv_color_hex(0x9CA3AF), 0);
    lv_obj_align(pomo_session_, LV_ALIGN_CENTER, 0, 2);

    // Nút Bắt đầu/Tạm dừng (chính), Đặt lại, Quay về đồng hồ.
    lv_obj_t* btn_toggle = make_button(pomo_cont_, "BAT DAU", 0x1A6FF5,
#if LV_FONT_MONTSERRAT_28
                                       &lv_font_montserrat_28,
#else
                                       nullptr,
#endif
                                       &on_pomo_toggle_, &btn_toggle_lbl_);
    lv_obj_set_size(btn_toggle, 200, 58);
    lv_obj_align(btn_toggle, LV_ALIGN_BOTTOM_MID, 0, -116);

    lv_obj_t* btn_reset = make_button(pomo_cont_, "DAT LAI", 0x374151, nullptr, &on_pomo_reset_);
    lv_obj_set_size(btn_reset, 132, 46);
    lv_obj_align(btn_reset, LV_ALIGN_BOTTOM_LEFT, 22, -44);

    lv_obj_t* btn_back = make_button(pomo_cont_, "DONG HO", 0x374151, nullptr, &on_pomo_back_);
    lv_obj_set_size(btn_back, 132, 46);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_RIGHT, -22, -44);
}

void LcdDisplay::ShowPomodoro(bool on) {
    if (pomo_cont_ == nullptr) return;
    DisplayLockGuard lock(this);
    if (on)
        lv_obj_clear_flag(pomo_cont_, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(pomo_cont_, LV_OBJ_FLAG_HIDDEN);
}

void LcdDisplay::UpdatePomodoro(const char* phase, const char* mmss, int pct, const char* session,
                                bool running) {
    if (pomo_time_ == nullptr) return;
    DisplayLockGuard lock(this);
    if (phase) lv_label_set_text(pomo_phase_, phase);
    if (mmss) lv_label_set_text(pomo_time_, mmss);
    if (session) lv_label_set_text(pomo_session_, session);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    lv_arc_set_value(pomo_arc_, pct);
    if (btn_toggle_lbl_) lv_label_set_text(btn_toggle_lbl_, running ? "TAM DUNG" : "BAT DAU");
}

void LcdDisplay::ShowClock(bool on) {
    if (clock_time_ == nullptr) return;
    DisplayLockGuard lock(this);
    // Bật đồng hồ -> ẩn logo (container_); tắt -> hiện lại logo.
    if (on) {
        if (container_) lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(clock_time_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(clock_date_, LV_OBJ_FLAG_HIDDEN);
        if (btn_pomo_) lv_obj_clear_flag(btn_pomo_, LV_OBJ_FLAG_HIDDEN);
    } else {
        if (container_) lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(clock_time_, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(clock_date_, LV_OBJ_FLAG_HIDDEN);
        if (btn_pomo_) lv_obj_add_flag(btn_pomo_, LV_OBJ_FLAG_HIDDEN);
    }
}

void LcdDisplay::UpdateClock(const char* time, const char* date) {
    if (clock_time_ == nullptr) return;
    DisplayLockGuard lock(this);
    if (time) lv_label_set_text(clock_time_, time);
    if (date) lv_label_set_text(clock_date_, date);
}

void LcdDisplay::SetWifiSignal(int bars) {
    if (wifi_bars_[0] == nullptr) return;
    DisplayLockGuard lock(this);
    for (int i = 0; i < 4; i++) {
        lv_color_t c;
        if (bars < 0) {
            c = lv_color_hex(0x803030);  // KHÔNG có WiFi -> 4 vạch đỏ mờ
        } else {
            c = (i < bars) ? lv_color_hex(0xFFFFFF) : lv_color_hex(0x303030);  // sáng theo số vạch
        }
        lv_obj_set_style_bg_color(wifi_bars_[i], c, 0);
    }
}

void LcdDisplay::SetStatus(const char* status) {
    if (status_label_ == nullptr) return;
    DisplayLockGuard lock(this);
    lv_label_set_text(status_label_, status ? status : "");
}

void LcdDisplay::ShowNotification(const char* notification, int duration_ms) {
    SetStatus(notification);
}

SpiLcdDisplay::SpiLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                             int width, int height, int offset_x, int offset_y, bool mirror_x,
                             bool mirror_y, bool swap_xy)
    : LcdDisplay(panel_io, panel, width, height) {
    // Bật panel (an toàn nếu panel không hỗ trợ disp_on_off).
    esp_err_t err = esp_lcd_panel_disp_on_off(panel_, true);
    if (err == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "panel khong ho tro disp_on_off; coi nhu ON");
    }

    ESP_LOGI(TAG, "Init LVGL");
    lv_init();

    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    port_cfg.task_priority = 1;
#if CONFIG_SOC_CPU_CORES_NUM > 1
    port_cfg.task_affinity = 1;
#endif
    lvgl_port_init(&port_cfg);

    ESP_LOGI(TAG, "Add LCD display %dx%d", width_, height_);
    // Partial-refresh, double-buffer ở SRAM nội DMA — chạy ổn, không treo. Double buffer giảm nhiễu/tearing.
    const lvgl_port_display_cfg_t display_cfg = {
        .io_handle = panel_io_,
        .panel_handle = panel_,
        .control_handle = nullptr,
        .buffer_size = static_cast<uint32_t>(width_ * 40),
        .double_buffer = true,
        .trans_size = 0,
        .hres = static_cast<uint32_t>(width_),
        .vres = static_cast<uint32_t>(height_),
        .monochrome = false,
        .rotation = {.swap_xy = swap_xy, .mirror_x = mirror_x, .mirror_y = mirror_y},
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = 1,
            .buff_spiram = 0,
            .sw_rotate = 0,
            .swap_bytes = 1,
            .full_refresh = 0,
            .direct_mode = 0,
        },
    };
    display_ = lvgl_port_add_disp(&display_cfg);
    if (display_ == nullptr) {
        ESP_LOGE(TAG, "lvgl_port_add_disp FAIL");
        return;
    }
    if (offset_x != 0 || offset_y != 0) {
        lv_display_set_offset(display_, offset_x, offset_y);
    }

    SetupUI();
    ESP_LOGI(TAG, "SpiLcdDisplay ready");
}
