#ifndef DISPLAY_H
#define DISPLAY_H

// Lớp Display nền — rút gọn theo kiến trúc Xiaozhi (bỏ emoji/theme/chat). Dùng LVGL.
#include <lvgl.h>
#include <esp_log.h>

#include <functional>
#include <string>

class Display {
public:
    Display() = default;
    virtual ~Display() = default;

    virtual void SetStatus(const char* status);
    virtual void ShowNotification(const char* notification, int duration_ms = 3000);
    // Cường độ WiFi góc trên trái: bars = -1 (không có WiFi) hoặc 0..4 (số vạch sáng).
    virtual void SetWifiSignal(int bars) {}

    // Màn đồng hồ: bật/tắt (thay logo) + cập nhật giờ/ngày.
    virtual void ShowClock(bool on) {}
    virtual void UpdateClock(const char* time, const char* date) {}

    // Màn Pomodoro: bật/tắt + cập nhật; đăng ký callback nút (mặc định no-op khi không có panel).
    virtual void ShowPomodoro(bool on) {}
    virtual void UpdatePomodoro(const char* phase, const char* mmss, int pct, const char* session,
                                bool running) {}
    virtual void OnPomoEnter(std::function<void()> cb) {}
    virtual void OnPomoBack(std::function<void()> cb) {}
    virtual void OnPomoToggle(std::function<void()> cb) {}
    virtual void OnPomoReset(std::function<void()> cb) {}

    inline int width() const { return width_; }
    inline int height() const { return height_; }

    // Khoá LVGL khi thao tác widget từ task khác (lvgl_port).
    virtual bool Lock(int timeout_ms = 0) = 0;
    virtual void Unlock() = 0;

protected:
    int width_ = 0;
    int height_ = 0;
};

// RAII khoá/mở LVGL.
class DisplayLockGuard {
public:
    explicit DisplayLockGuard(Display* display) : display_(display) {
        if (!display_->Lock(30000)) {
            ESP_LOGE("Display", "Failed to lock display");
        }
    }
    ~DisplayLockGuard() { display_->Unlock(); }

private:
    Display* display_;
};

// Display rỗng (an toàn khi không có panel).
class NoDisplay : public Display {
public:
    bool Lock(int timeout_ms = 0) override { return true; }
    void Unlock() override {}
};

#endif  // DISPLAY_H
