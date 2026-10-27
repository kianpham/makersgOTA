#include "application.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>

#include "app_config.h"
#include "board.h"
#include "display.h"
#include "ota.h"
#include "protocols/ws_client.h"
#include "timesync.h"

#define TAG "Application"

// Lệnh nút Pomodoro (đặt từ task LVGL, xử lý ở task chính).
enum { kCmdNone = 0, kCmdEnter, kCmdBack, kCmdToggle, kCmdReset };

// Số vạch sóng WiFi từ RSSI: -1 = không nối, 1..4 = yếu->mạnh.
static int wifi_bars() {
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) return -1;
    int r = ap.rssi;
    if (r >= -55) return 4;
    if (r >= -65) return 3;
    if (r >= -72) return 2;
    return 1;
}

Application::Application() = default;
Application::~Application() = default;

void Application::Initialize() {
    ESP_LOGI(TAG, "===== MakerSG (C++/Xiaozhi-arch) khoi dong v%s =====", FW_APP_VERSION);
    auto& board = Board::GetInstance();
    auto display = board.GetDisplay();
    ESP_LOGI(TAG, "Board=%s uuid=%s", board.GetBoardType().c_str(), board.GetUuid().c_str());
    if (display) display->SetStatus("BOOTING");

    // Nút Pomodoro chạy trong task LVGL -> chỉ đặt lệnh qua atomic, task chính (Run) xử lý.
    if (display) {
        display->OnPomoEnter([this] { pomo_cmd_.store(kCmdEnter); });
        display->OnPomoBack([this] { pomo_cmd_.store(kCmdBack); });
        display->OnPomoToggle([this] { pomo_cmd_.store(kCmdToggle); });
        display->OnPomoReset([this] { pomo_cmd_.store(kCmdReset); });
    }

    // 1. Mạng: provisioning (nếu chưa có WiFi) hoặc STA. Hàm này có thể reboot.
    state_ = kAppConfiguring;
    board.StartNetwork();
    state_ = kAppConnecting;
    if (display) display->SetWifiSignal(wifi_bars());  // WiFi đã nối -> hiện vạch sóng
    timesync_start();                                  // SNTP (non-blocking)

    // 2. OTA check + WS (contract PHASE1.md). KHÔNG chặn: nếu server không tới được / chưa đăng ký
    //    -> vẫn CHUYỂN TIẾP vào màn chờ (đồng hồ), Run() sẽ thử lại ở nền mỗi 30s.
    if (display) display->SetStatus("OTACHECKING");
    backend_ready_ = ConnectBackend(display);
    if (!backend_ready_) {
        ESP_LOGW(TAG, "OTA/WS chua san sang -> vao man cho, thu lai o nen");
        if (display) display->SetStatus("OFFLINE");
    }

    ESP_LOGI(TAG, "===== Initialize xong (backend_ready=%d) =====", backend_ready_);
    state_ = kAppIdle;
}

// OTA check một lần; nếu có bản mới -> update + restart; nếu có websocket -> connect. KHÔNG retry chặn.
bool Application::ConnectBackend(Display* display) {
    Ota ota;
    if (!ota.CheckVersion()) {
        // Không lấy được websocket: hoặc server lỗi/không tới được, hoặc thiết bị chưa đăng ký.
        if (ota.HasActivationCode() && display) {
            std::string msg = "CODE " + ota.GetActivationCode();
            display->ShowNotification(msg.c_str());
        }
        return false;
    }

    // Firmware mới? -> OTA update rồi restart.
    if (ota.HasNewVersion()) {
        if (display) display->SetStatus("OTA UPDATE");
        bool ok = ota.Upgrade(ota.GetFirmwareUrl(), [display](int pct) {
            if (display) {
                char buf[16];
                snprintf(buf, sizeof(buf), "OTA %d%%", pct);
                display->SetStatus(buf);
            }
        });
        if (ok) {
            ESP_LOGI(TAG, "OTA OK -> restart");
            esp_restart();
        }
    }

    // WebSocket connect + hello.
    if (display) display->SetStatus("WS CONNECTING");
    ws_ = std::make_unique<WsClient>();
    ws_->OnConnected([display]() {
        if (display) display->SetStatus("ONLINE");
    });
    ws_->OnDisconnected([display]() {
        if (display) display->SetStatus("OFFLINE");
    });
    ws_->Start(ota.GetWsUrl(), Ota::GetDeviceId(), ota.GetWsToken());
    ESP_LOGI(TAG, "===== Backend OK: OTA+WS =====");
    return true;
}

// Vẽ lại màn Pomodoro từ trạng thái pomo_ hiện tại (gọi ở task chính).
void Application::RenderPomodoro(Display* display) {
    if (!display) return;
    int rem = pomo_.remaining();
    if (rem < 0) rem = 0;
    int mm = rem / 60;
    if (mm > 99) mm = 99;  // kẹp để ô MM:SS luôn vừa buffer
    char mmss[8];
    snprintf(mmss, sizeof(mmss), "%02d:%02d", mm, rem % 60);
    int dur = pomo_.duration();
    int pct = (dur > 0) ? (dur - rem) * 100 / dur : 0;
    const char* phase = "";
    switch (pomo_.phase()) {
        case Pomodoro::kWork:
            phase = "LAM VIEC";
            break;
        case Pomodoro::kShortBreak:
            phase = "NGHI NGAN";
            break;
        case Pomodoro::kLongBreak:
            phase = "NGHI DAI";
            break;
    }
    char sess[16];
    snprintf(sess, sizeof(sess), "Phien %d/%d", pomo_.work_index(),
             Pomodoro::sessions_before_long());
    display->UpdatePomodoro(phase, mmss, pct, sess, pomo_.running());
}

void Application::Run() {
    // Vòng UI nhịp 200ms: xử lý nút Pomodoro (độ trễ <=200ms) + công việc mỗi giây (đồng hồ/pomodoro/wifi).
    auto display = Board::GetInstance().GetDisplay();
    int last_min = -1;
    bool clock_on = false;
    int sub = 0;  // đếm nhịp 200ms; 5 nhịp = 1s
    int wifi_tick = 0;
    int backend_tick = 0;
    while (true) {
        // 1) Lệnh nút Pomodoro (đặt từ task LVGL qua atomic) — xử lý ở task này để pomo_ chỉ bị 1 task chạm.
        int cmd = pomo_cmd_.exchange(kCmdNone);
        if (cmd == kCmdEnter) {
            screen_ = kScreenPomo;
            if (display) display->ShowPomodoro(true);
            RenderPomodoro(display);
        } else if (cmd == kCmdBack) {
            screen_ = kScreenClock;
            if (display) display->ShowPomodoro(false);
        } else if (cmd == kCmdToggle) {
            pomo_.Toggle();
            RenderPomodoro(display);
        } else if (cmd == kCmdReset) {
            pomo_.Reset();
            RenderPomodoro(display);
        }

        // 2) Công việc mỗi ~1s.
        if (++sub >= 5) {
            sub = 0;
            if (screen_ == kScreenPomo) {
                pomo_.Tick(1);
                RenderPomodoro(display);
            } else {
                struct tm t;
                if (timesync_get_local(&t)) {
                    if (!clock_on) {
                        if (display) display->ShowClock(true);
                        clock_on = true;
                        last_min = -1;
                    }
                    char tbuf[16];
                    snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
                    if (t.tm_hour * 60 + t.tm_min != last_min) {  // ngày đổi theo phút (nhẹ)
                        last_min = t.tm_hour * 60 + t.tm_min;
                        char dbuf[24];
                        strftime(dbuf, sizeof(dbuf), "%a %d-%m-%Y", &t);
                        if (display) display->UpdateClock(tbuf, dbuf);
                    } else {
                        if (display) display->UpdateClock(tbuf, nullptr);
                    }
                }
            }
            if (display && (++wifi_tick % 3) == 0) display->SetWifiSignal(wifi_bars());  // mỗi ~3s
            // Backend chưa sẵn sàng -> thử lại ở nền mỗi ~30s, không chặn màn chờ.
            if (!backend_ready_ && (++backend_tick % 30) == 0) {
                ESP_LOGI(TAG, "Thu ket noi backend lai (nen)...");
                backend_ready_ = ConnectBackend(display);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
