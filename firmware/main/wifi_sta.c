// WiFi STA — kết nối từ NVS (Phase 1).
#include "wifi_sta.h"

#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs.h"

static const char *TAG = "sta";
static EventGroupHandle_t s_eg;
#define CONNECTED_BIT BIT0
#define FAIL_BIT BIT1
static int s_retry;
#define MAX_RETRY 5

static void evt(void *arg, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry++ < MAX_RETRY) {
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_eg, FAIL_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_retry = 0;
        xEventGroupSetBits(s_eg, CONNECTED_BIT);
    }
}

bool wifi_sta_connect_from_nvs(int timeout_ms) {
    char ssid[64] = {0}, pass[64] = {0};
    size_t sl = sizeof(ssid), pl = sizeof(pass);
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READONLY, &h) != ESP_OK) return false;
    esp_err_t e1 = nvs_get_str(h, "ssid", ssid, &sl);
    nvs_get_str(h, "pass", pass, &pl);
    nvs_close(h);
    if (e1 != ESP_OK) return false;

    s_eg = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &evt, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &evt, NULL, NULL);

    wifi_config_t sta = {0};
    strncpy((char *)sta.sta.ssid, ssid, sizeof(sta.sta.ssid));
    strncpy((char *)sta.sta.password, pass, sizeof(sta.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_eg, CONNECTED_BIT | FAIL_BIT, pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(timeout_ms));
    bool ok = (bits & CONNECTED_BIT) != 0;
    ESP_LOGI(TAG, "%s (SSID=%s)", ok ? "Connected" : "Failed/timeout", ssid);
    return ok;
}
