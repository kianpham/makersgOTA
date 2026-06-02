// OTA client — POST /ota, parse JSON, esp_https_ota nếu có bản mới (Phase 1).
#include "ota_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_config.h"
#include "cJSON.h"
#include "display.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"

static const char *TAG = "ota";

static void get_device_id(char *out, size_t n) {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(out, n, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// POST JSON, đọc response vào buffer cấp phát. Trả buffer (caller free) hoặc NULL.
static char *http_post_json(const char *url, const char *device_id, const char *body, int *status_out) {
    esp_http_client_config_t cfg = {.url = url, .method = HTTP_METHOD_POST, .timeout_ms = 10000};
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    esp_http_client_set_header(c, "Content-Type", "application/json");
    esp_http_client_set_header(c, "device-id", device_id);

    if (esp_http_client_open(c, strlen(body)) != ESP_OK) {
        esp_http_client_cleanup(c);
        return NULL;
    }
    esp_http_client_write(c, body, strlen(body));
    int clen = esp_http_client_fetch_headers(c);
    if (status_out) *status_out = esp_http_client_get_status_code(c);

    int cap = (clen > 0 && clen < 8192) ? clen + 1 : 8192;
    char *buf = malloc(cap);
    if (!buf) {
        esp_http_client_cleanup(c);
        return NULL;
    }
    int total = 0, r;
    while ((r = esp_http_client_read(c, buf + total, cap - 1 - total)) > 0) {
        total += r;
        if (total >= cap - 1) break;
    }
    buf[total] = 0;
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    return buf;
}

bool ota_client_run(ota_result_t *out) {
    out->have_ws = false;
    char device_id[24];
    get_device_id(device_id, sizeof(device_id));

    char body[192];
    snprintf(body, sizeof(body),
             "{\"application\":{\"version\":\"%s\"},\"board\":{\"type\":\"%s\",\"name\":\"%s\"},"
             "\"chip_model_name\":\"%s\"}",
             FW_APP_VERSION, BOARD_TYPE, BOARD_NAME, BOARD_TYPE);

    int status = 0;
    char *resp = http_post_json(OTA_URL, device_id, body, &status);
    if (!resp) {
        ESP_LOGE(TAG, "OTA request that bai");
        return false;
    }
    ESP_LOGI(TAG, "OTA status=%d", status);

    cJSON *root = cJSON_Parse(resp);
    free(resp);
    if (!root) {
        ESP_LOGE(TAG, "OTA JSON parse loi");
        return false;
    }

    // Chưa đăng ký → hiện mã activation lên màn, dừng.
    cJSON *act = cJSON_GetObjectItem(root, "activation");
    if (cJSON_IsObject(act)) {
        const char *code = cJSON_GetStringValue(cJSON_GetObjectItem(act, "code"));
        ESP_LOGW(TAG, "Thiet bi CHUA dang ky. Ma kich hoat: %s", code ? code : "?");
        display_show_code(code);  // hiện mã to lên LCD
        cJSON_Delete(root);
        return false;
    }

    // Firmware mới? → OTA granular để báo % lên màn.
    cJSON *fw = cJSON_GetObjectItem(root, "firmware");
    if (cJSON_IsObject(fw)) {
        const char *ver = cJSON_GetStringValue(cJSON_GetObjectItem(fw, "version"));
        const char *url = cJSON_GetStringValue(cJSON_GetObjectItem(fw, "url"));
        if (ver && url && strcmp(ver, FW_APP_VERSION) != 0) {
            ESP_LOGI(TAG, "Co firmware moi %s -> OTA tu %s", ver, url);
            esp_http_client_config_t ota_http = {
                .url = url, .timeout_ms = 30000, .keep_alive_enable = true};
            esp_https_ota_config_t ota_cfg = {.http_config = &ota_http};

            esp_https_ota_handle_t h = NULL;
            esp_err_t e = esp_https_ota_begin(&ota_cfg, &h);
            if (e == ESP_OK) {
                display_ota_begin();
                int total = esp_https_ota_get_image_size(h);
                while ((e = esp_https_ota_perform(h)) == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
                    int read = esp_https_ota_get_image_len_read(h);
                    display_progress(total > 0 ? (int)((int64_t)read * 100 / total) : 0);
                }
                if (e == ESP_OK && esp_https_ota_is_complete_data_received(h) &&
                    esp_https_ota_finish(h) == ESP_OK) {
                    display_progress(100);
                    display_status("OTA OK", "REBOOTING", NULL);
                    ESP_LOGI(TAG, "OTA OK -> restart");
                    cJSON_Delete(root);
                    esp_restart();
                }
                esp_https_ota_abort(h);
            }
            ESP_LOGE(TAG, "OTA that bai: %s", esp_err_to_name(e));
            display_status("OTA FAILED", esp_err_to_name(e), NULL);
        }
    }

    // WebSocket url + token
    cJSON *ws = cJSON_GetObjectItem(root, "websocket");
    if (cJSON_IsObject(ws)) {
        const char *url = cJSON_GetStringValue(cJSON_GetObjectItem(ws, "url"));
        const char *tok = cJSON_GetStringValue(cJSON_GetObjectItem(ws, "token"));
        if (url) strncpy(out->ws_url, url, sizeof(out->ws_url) - 1);
        if (tok) strncpy(out->ws_token, tok, sizeof(out->ws_token) - 1);
        out->have_ws = (url != NULL);
    }

    cJSON_Delete(root);
    return out->have_ws;
}
