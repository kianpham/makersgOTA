#ifndef APPLICATION_H
#define APPLICATION_H

// Application MakerSG (tự viết, KHÔNG audio). Singleton, state machine riêng.
// Phase A: khởi tạo Board + Display, hiện UI. Phase B: provisioning→OTA→WS. Phase C: đồng hồ/pomodoro.
#include <atomic>
#include <memory>
#include <string>

#include "pomodoro.h"

class WsClient;
class Display;

enum AppState {
    kAppBooting,
    kAppConfiguring,  // provisioning WiFi
    kAppConnecting,   // STA + OTA + WS
    kAppIdle,         // màn chính (đồng hồ)
};

// Màn đang hiển thị ở trạng thái idle.
enum Screen {
    kScreenClock,
    kScreenPomo,
};

class Application {
public:
    static Application& GetInstance() {
        static Application instance;
        return instance;
    }
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void Initialize();  // dựng board + display
    void Run();         // vòng lặp chính, không return

    AppState state() const { return state_; }

private:
    Application();
    ~Application();

    // OTA check + (nếu có bản mới) update + WS connect. Trả true khi đã lấy được websocket config.
    // KHÔNG chặn lâu: thất bại -> trả false để luồng chính vào màn chờ và thử lại ở nền.
    bool ConnectBackend(Display* display);

    // Vẽ lại màn Pomodoro từ trạng thái pomo_ hiện tại.
    void RenderPomodoro(Display* display);

    AppState state_ = kAppBooting;
    bool backend_ready_ = false;  // đã lấy được websocket + tạo ws_ chưa
    std::unique_ptr<WsClient> ws_;

    // Pomodoro: logic ở task chính; nút bấm (task LVGL) chỉ đặt lệnh qua atomic để tránh tranh chấp.
    Pomodoro pomo_;
    Screen screen_ = kScreenClock;
    std::atomic<int> pomo_cmd_{0};
};

#endif  // APPLICATION_H
