// OTA client (port ota_client.c) — POST /ota, parse JSON, esp_https_ota nếu có bản mới.
#include "ota.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <cJSON.h>
#include <esp_http_client.h>
#include <esp_https_ota.h>
#include <esp_log.h>
#include <esp_mac.h>

#include "app_config.h"

#define TAG "Ota"

std::string Ota::GetDeviceId() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char s[24];
    snprintf(s, sizeof(s), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4],
             mac[5]);
    return std::string(s);
}

// POST JSON, đọc response vào buffer cấp phát (caller free). Trả buffer hoặc nullptr.
static char* http_post_json(const char* url, const char* device_id, const char* body,
                            int* status_out) {
    esp_http_client_config_t cfg = {};
    cfg.url = url;
    cfg.method = HTTP_METHOD_POST;
    cfg.timeout_ms = 10000;
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    esp_http_client_set_header(c, "Content-Type", "application/json");
    esp_http_client_set_header(c, "device-id", device_id);

    if (esp_http_client_open(c, strlen(body)) != ESP_OK) {
        esp_http_client_cleanup(c);
        return nullptr;
    }
    esp_http_client_write(c, body, strlen(body));
    int clen = esp_http_client_fetch_headers(c);
    if (status_out) *status_out = esp_http_client_get_status_code(c);

    int cap = (clen > 0 && clen < 8192) ? clen + 1 : 8192;
    char* buf = (char*)malloc(cap);
    if (!buf) {
        esp_http_client_cleanup(c);
        return nullptr;
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

bool Ota::CheckVersion() {
    has_activation_code_ = has_websocket_ = has_new_version_ = false;
    std::string device_id = GetDeviceId();

    char body[192];
    snprintf(body, sizeof(body),
             "{\"application\":{\"version\":\"%s\"},\"board\":{\"type\":\"%s\",\"name\":\"%s\"},"
             "\"chip_model_name\":\"%s\"}",
             FW_APP_VERSION, BOARD_TYPE, BOARD_NAME, BOARD_TYPE);

    int status = 0;
    char* resp = http_post_json(OTA_URL, device_id.c_str(), body, &status);
    if (!resp) {
        ESP_LOGE(TAG, "OTA request that bai");
        return false;
    }
    ESP_LOGI(TAG, "OTA status=%d", status);

    cJSON* root = cJSON_Parse(resp);
    free(resp);
    if (!root) {
        ESP_LOGE(TAG, "OTA JSON parse loi");
        return false;
    }

    // Chưa đăng ký -> activation.code
    cJSON* act = cJSON_GetObjectItem(root, "activation");
    if (cJSON_IsObject(act)) {
        const char* code = cJSON_GetStringValue(cJSON_GetObjectItem(act, "code"));
        if (code) {
            activation_code_ = code;
            has_activation_code_ = true;
            ESP_LOGW(TAG, "Thiet bi CHUA dang ky. Ma: %s", code);
        }
    }

    // firmware mới?
    cJSON* fw = cJSON_GetObjectItem(root, "firmware");
    if (cJSON_IsObject(fw)) {
        const char* ver = cJSON_GetStringValue(cJSON_GetObjectItem(fw, "version"));
        const char* url = cJSON_GetStringValue(cJSON_GetObjectItem(fw, "url"));
        if (ver && url && strcmp(ver, FW_APP_VERSION) != 0) {
            firmware_version_ = ver;
            firmware_url_ = url;
            has_new_version_ = true;
        }
    }

    // websocket url + token
    cJSON* ws = cJSON_GetObjectItem(root, "websocket");
    if (cJSON_IsObject(ws)) {
        const char* url = cJSON_GetStringValue(cJSON_GetObjectItem(ws, "url"));
        const char* tok = cJSON_GetStringValue(cJSON_GetObjectItem(ws, "token"));
        if (url) {
            ws_url_ = url;
            if (tok) ws_token_ = tok;
            has_websocket_ = true;
        }
    }

    cJSON_Delete(root);
    return has_websocket_;
}

bool Ota::Upgrade(const std::string& url, std::function<void(int)> progress) {
    ESP_LOGI(TAG, "OTA tu %s", url.c_str());
    esp_http_client_config_t http = {};
    http.url = url.c_str();
    http.timeout_ms = 30000;
    http.keep_alive_enable = true;
    esp_https_ota_config_t ota_cfg = {};
    ota_cfg.http_config = &http;

    esp_https_ota_handle_t h = nullptr;
    if (esp_https_ota_begin(&ota_cfg, &h) != ESP_OK) {
        ESP_LOGE(TAG, "https_ota_begin fail");
        return false;
    }
    int total = esp_https_ota_get_image_size(h);
    esp_err_t e;
    while ((e = esp_https_ota_perform(h)) == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
        int read = esp_https_ota_get_image_len_read(h);
        if (progress && total > 0) progress((int)((int64_t)read * 100 / total));
    }
    if (e == ESP_OK && esp_https_ota_is_complete_data_received(h) &&
        esp_https_ota_finish(h) == ESP_OK) {
        if (progress) progress(100);
        ESP_LOGI(TAG, "OTA OK");
        return true;
    }
    esp_https_ota_abort(h);
    ESP_LOGE(TAG, "OTA that bai: %s", esp_err_to_name(e));
    return false;
}
