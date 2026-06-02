#pragma once
#include <stdbool.h>

// Có cấu hình WiFi (SSID) trong NVS chưa?
bool wifi_has_credentials(void);

// Khởi động SoftAP + captive portal nhập SSID/pass; block tới khi người dùng lưu.
// Sau khi trả về nên esp_restart() để vào luồng STA.
void wifi_provision_start_blocking(void);
