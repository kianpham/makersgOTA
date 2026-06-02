#pragma once

// Phiên bản firmware hiện tại — gửi trong OTA check; server so để quyết định update.
#define FW_APP_VERSION "1.0.0"

// Định danh board (gửi trong body OTA).
#define BOARD_NAME "es3c35p"
#define BOARD_TYPE "esp32s3"

// Endpoint OTA của server Phase 1 (cloud/). ĐỔI sang IP LAN máy chạy cloud khi test board thật.
//   vd: "http://192.168.1.100:8000/api/v1/ota"
#define OTA_URL "http://192.168.1.100:8000/api/v1/ota"

// SoftAP provisioning (khi chưa có WiFi). Pass rỗng = AP mở; nếu đặt phải >= 8 ký tự.
#define PROV_AP_SSID "Xiaozhi-Setup"
#define PROV_AP_PASS ""

// Thời gian chờ STA kết nối trước khi quay lại provisioning (ms).
#define WIFI_STA_TIMEOUT_MS 20000
