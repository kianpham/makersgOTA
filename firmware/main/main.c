// Phase 1 orchestration: provisioning WiFi -> STA -> OTA check -> WebSocket.
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "display.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "ota_client.h"
#include "wifi_provision.h"
#include "wifi_sta.h"
#include "ws_client.h"

static const char *TAG = "main";

static void common_init(void) {
    esp_err_t e = nvs_flash_init();
    if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wcfg));
}

static void device_id_str(char *out, size_t n) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(out, n, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void app_main(void) {
    common_init();
    display_init();  // LCD (an toàn nếu fail → chạy headless)
    display_status("MAKERSG", "BOOTING", NULL);

    // 1. Provisioning nếu chưa có WiFi.
    if (!wifi_has_credentials()) {
        ESP_LOGI(TAG, "Chua co WiFi -> provisioning (SoftAP)");
        display_status("WIFI SETUP", PROV_AP_SSID, "192.168.4.1");
        wifi_provision_start_blocking();
        ESP_LOGI(TAG, "Da luu WiFi -> restart");
        esp_restart();
    }

    // 2. Kết nối STA.
    display_status("WIFI", "CONNECTING", NULL);
    if (!wifi_sta_connect_from_nvs(WIFI_STA_TIMEOUT_MS)) {
        ESP_LOGW(TAG, "Khong ket noi duoc WiFi -> quay lai provisioning");
        display_status("WIFI SETUP", PROV_AP_SSID, "192.168.4.1");
        wifi_provision_start_blocking();
        esp_restart();
    }

    // 3. OTA check (tự update firmware nếu có bản mới) -> lấy ws url+token.
    display_status("OTA", "CHECKING", NULL);
    ota_result_t res = {0};
    if (!ota_client_run(&res)) {
        ESP_LOGW(TAG, "Chua co ws (co the chua dang ky). Thu lai moi 10s...");
        for (int i = 0; i < 30 && !res.have_ws; ++i) {
            vTaskDelay(pdMS_TO_TICKS(10000));
            ota_client_run(&res);
        }
        if (!res.have_ws) {
            ESP_LOGE(TAG, "Van chua co ws -> dung.");
            return;
        }
    }

    // 4. Kết nối WebSocket.
    char device_id[24];
    device_id_str(device_id, sizeof(device_id));
    display_status("WS", "CONNECTING", NULL);
    ws_client_start(res.ws_url, device_id, res.ws_token);

    ESP_LOGI(TAG, "Phase 1 core OK: WiFi + OTA + WS da ket noi (device-id=%s)", device_id);
}
