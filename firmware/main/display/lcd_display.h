#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

// LcdDisplay LVGL rút gọn (không font/emoji/theme của Xiaozhi). Đủ để dựng UI MakerSG
// (logo/đồng hồ/pomodoro) bằng LVGL widget. Tự kiểm soát, deps tối thiểu.
#include "display.h"

#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <lvgl.h>

#include <functional>

class LcdDisplay : public Display {
public:
    ~LcdDisplay();

    void SetStatus(const char* status) override;
    void ShowNotification(const char* notification, int duration_ms = 3000) override;
    void SetWifiSignal(int bars) override;
    void ShowClock(bool on) override;
    void UpdateClock(const char* time, const char* date) override;

    // ----- Pomodoro -----
    // Hiện/ẩn màn Pomodoro (ẩn logo + đồng hồ khi bật).
    void ShowPomodoro(bool on) override;
    // Cập nhật màn Pomodoro: nhãn phase, "MM:SS", % tiến trình (0..100), "Phien n/4", đang chạy?
    void UpdatePomodoro(const char* phase, const char* mmss, int pct, const char* session,
                        bool running) override;
    // Đăng ký callback nút bấm (chạy trong task LVGL — handler chỉ nên thao tác nhẹ/atomic).
    void OnPomoEnter(std::function<void()> cb) override { on_pomo_enter_ = std::move(cb); }
    void OnPomoBack(std::function<void()> cb) override { on_pomo_back_ = std::move(cb); }
    void OnPomoToggle(std::function<void()> cb) override { on_pomo_toggle_ = std::move(cb); }
    void OnPomoReset(std::function<void()> cb) override { on_pomo_reset_ = std::move(cb); }

    // Truy cập root screen để màn UI (đồng hồ/pomodoro) dựng widget trực tiếp.
    lv_obj_t* root() { return lv_screen_active(); }

protected:
    LcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width, int height);

    bool Lock(int timeout_ms = 0) override;
    void Unlock() override;

    void SetupUI();

    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    lv_display_t* display_ = nullptr;

    lv_obj_t* container_ = nullptr;
    lv_obj_t* title_label_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* wifi_bars_[4] = {nullptr, nullptr, nullptr, nullptr};
    lv_obj_t* clock_time_ = nullptr;
    lv_obj_t* clock_date_ = nullptr;
    lv_obj_t* btn_pomo_ = nullptr;  // nút mở Pomodoro (trên màn đồng hồ)

    // Pomodoro view
    lv_obj_t* pomo_cont_ = nullptr;
    lv_obj_t* pomo_phase_ = nullptr;
    lv_obj_t* pomo_arc_ = nullptr;
    lv_obj_t* pomo_time_ = nullptr;
    lv_obj_t* pomo_session_ = nullptr;
    lv_obj_t* btn_toggle_lbl_ = nullptr;

    std::function<void()> on_pomo_enter_;
    std::function<void()> on_pomo_back_;
    std::function<void()> on_pomo_toggle_;
    std::function<void()> on_pomo_reset_;

    void SetupPomodoroUI(lv_obj_t* screen);
};

// LCD qua SPI/QSPI (panel đã tạo sẵn ở board), bọc vào lvgl_port.
class SpiLcdDisplay : public LcdDisplay {
public:
    SpiLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width,
                  int height, int offset_x, int offset_y, bool mirror_x, bool mirror_y, bool swap_xy);
};

#endif  // LCD_DISPLAY_H
