#pragma once

// Hằng số ứng dụng MakerSG (Phase B). Giữ contract PHASE1.md với cloud FastAPI.
#define FW_APP_VERSION "1.0.0"
#define BOARD_NAME "es3c35p"
#define BOARD_TYPE "esp32s3"

// Endpoint OTA của cloud. ĐỔI sang IP LAN máy chạy cloud khi test board thật.
#define OTA_URL "http://192.168.1.13:8000/api/v1/ota"

// SoftAP provisioning (khi chưa có WiFi). Pass rỗng = AP mở; nếu đặt phải >= 8 ký tự.
#define PROV_AP_SSID "makersgHUB"
#define PROV_AP_PASS ""

// Thời gian chờ STA kết nối trước khi quay lại provisioning (ms).
#define WIFI_STA_TIMEOUT_MS 20000
