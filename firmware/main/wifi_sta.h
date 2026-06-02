#pragma once
#include <stdbool.h>

// Kết nối STA từ SSID/pass trong NVS. true nếu lấy được IP trong timeout_ms.
bool wifi_sta_connect_from_nvs(int timeout_ms);
