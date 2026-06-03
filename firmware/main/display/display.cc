#include "display.h"

#define TAG "Display"

// Hành vi mặc định: ghi vào label trạng thái (lớp con LcdDisplay override phần vẽ widget).
void Display::SetStatus(const char* status) { ESP_LOGI(TAG, "status: %s", status ? status : ""); }

void Display::ShowNotification(const char* notification, int duration_ms) {
    ESP_LOGI(TAG, "notify: %s", notification ? notification : "");
}
