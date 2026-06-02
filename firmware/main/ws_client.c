// WebSocket client — kết nối + gửi hello (Phase 1). Dùng managed component esp_websocket_client.
#include "ws_client.h"

#include <stdio.h>
#include <string.h>

#include "display.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "ws";
static esp_websocket_client_handle_t s_client;

static void ws_event(void *arg, esp_event_base_t base, int32_t id, void *data) {
    esp_websocket_event_data_t *d = (esp_websocket_event_data_t *)data;
    switch (id) {
        case WEBSOCKET_EVENT_CONNECTED: {
            ESP_LOGI(TAG, "WS connected -> gui hello");
            const char *hello = "{\"type\":\"hello\",\"version\":1,\"transport\":\"websocket\"}";
            esp_websocket_client_send_text(s_client, hello, strlen(hello), portMAX_DELAY);
            display_status("ONLINE", "DEVICE READY", NULL);
            break;
        }
        case WEBSOCKET_EVENT_DATA:
            if (d && d->op_code == 0x1 && d->data_len > 0) {
                ESP_LOGI(TAG, "WS recv: %.*s", d->data_len, (char *)d->data_ptr);
            }
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "WS disconnected");
            display_status("OFFLINE", "RECONNECTING", NULL);
            break;
        default:
            break;
    }
}

void ws_client_start(const char *url, const char *device_id, const char *token) {
    static char headers[640];
    snprintf(headers, sizeof(headers), "device-id: %s\r\nAuthorization: Bearer %s\r\n", device_id,
             token);

    esp_websocket_client_config_t cfg = {
        .uri = url,
        .headers = headers,
        .reconnect_timeout_ms = 5000,
        .network_timeout_ms = 10000,
    };
    s_client = esp_websocket_client_init(&cfg);
    esp_websocket_register_events(s_client, WEBSOCKET_EVENT_ANY, ws_event, NULL);
    esp_websocket_client_start(s_client);
    ESP_LOGI(TAG, "WS connecting %s", url);
}
