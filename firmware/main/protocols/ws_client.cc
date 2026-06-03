#include "ws_client.h"

#include <cstdio>
#include <cstring>

#include <esp_log.h>

#define TAG "WsClient"

void WsClient::EventHandler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    auto* self = static_cast<WsClient*>(arg);
    auto* d = static_cast<esp_websocket_event_data_t*>(data);
    switch (id) {
        case WEBSOCKET_EVENT_CONNECTED: {
            ESP_LOGI(TAG, "WS connected -> gui hello");
            const char* hello = "{\"type\":\"hello\",\"version\":1,\"transport\":\"websocket\"}";
            esp_websocket_client_send_text(self->client_, hello, strlen(hello), portMAX_DELAY);
            if (self->on_connected_) self->on_connected_();
            break;
        }
        case WEBSOCKET_EVENT_DATA:
            if (d && d->op_code == 0x1 && d->data_len > 0) {
                ESP_LOGI(TAG, "WS recv: %.*s", d->data_len, (char*)d->data_ptr);
            }
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "WS disconnected");
            if (self->on_disconnected_) self->on_disconnected_();
            break;
        default:
            break;
    }
}

void WsClient::Start(const std::string& url, const std::string& device_id,
                     const std::string& token) {
    static char headers[640];
    snprintf(headers, sizeof(headers), "device-id: %s\r\nAuthorization: Bearer %s\r\n",
             device_id.c_str(), token.c_str());

    esp_websocket_client_config_t cfg = {};
    cfg.uri = url.c_str();
    cfg.headers = headers;
    cfg.reconnect_timeout_ms = 5000;
    cfg.network_timeout_ms = 10000;

    client_ = esp_websocket_client_init(&cfg);
    esp_websocket_register_events(client_, WEBSOCKET_EVENT_ANY, EventHandler, this);
    esp_websocket_client_start(client_);
    ESP_LOGI(TAG, "WS connecting %s", url.c_str());
}
