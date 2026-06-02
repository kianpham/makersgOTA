#pragma once
#include <stdbool.h>

typedef struct {
    char ws_url[256];
    char ws_token[512];
    bool have_ws;
} ota_result_t;

// Gọi OTA_URL với device-id (MAC). Nếu có firmware mới → tải+flash (esp_https_ota) rồi restart.
// Nếu thiết bị chưa đăng ký → log mã activation, trả false.
// Thành công (đã đăng ký) → điền ws_url + ws_token vào out, trả true.
bool ota_client_run(ota_result_t *out);
