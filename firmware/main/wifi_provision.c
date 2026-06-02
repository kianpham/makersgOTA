// WiFi provisioning — SoftAP + captive portal (Phase 1).
// Lưu SSID/pass vào NVS namespace "wifi". Sau khi lưu, set bit để main restart sang STA.
#include "wifi_provision.h"

#include <string.h>

#include "app_config.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs.h"

static const char *TAG = "prov";
static EventGroupHandle_t s_eg;
#define PROV_DONE_BIT BIT0

bool wifi_has_credentials(void) {
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READONLY, &h) != ESP_OK) return false;
    size_t len = 0;
    bool ok = (nvs_get_str(h, "ssid", NULL, &len) == ESP_OK) && len > 1;
    nvs_close(h);
    return ok;
}

static esp_err_t root_get(httpd_req_t *req) {
    static const char *page =
        "<!doctype html><meta name=viewport content='width=device-width,initial-scale=1'>"
        "<h2>Xiaozhi WiFi Setup</h2>"
        "<form method=POST action=/save>"
        "SSID:<br><input name=ssid><br>Password:<br><input name=pass type=password><br><br>"
        "<button type=submit>Luu &amp; Ket noi</button></form>";
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t save_post(httpd_req_t *req) {
    char buf[256] = {0};
    int total = req->content_len < (int)sizeof(buf) - 1 ? req->content_len : (int)sizeof(buf) - 1;
    int recvd = 0;
    while (recvd < total) {
        int r = httpd_req_recv(req, buf + recvd, total - recvd);
        if (r <= 0) return ESP_FAIL;
        recvd += r;
    }
    buf[recvd] = 0;

    char ssid[64] = {0}, pass[64] = {0};
    httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid));
    httpd_query_key_value(buf, "pass", pass, sizeof(pass));
    // TODO: URL-decode đầy đủ (Phase 1 giả định SSID/pass không chứa %xx, '+').

    if (strlen(ssid) == 0) {
        httpd_resp_send(req, "SSID trong", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    nvs_handle_t h;
    nvs_open("wifi", NVS_READWRITE, &h);
    nvs_set_str(h, "ssid", ssid);
    nvs_set_str(h, "pass", pass);
    nvs_commit(h);
    nvs_close(h);
    ESP_LOGI(TAG, "Da luu SSID=%s", ssid);

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, "<h3>Da luu. Thiet bi se khoi dong lai va ket noi.</h3>",
                    HTTPD_RESP_USE_STRLEN);
    xEventGroupSetBits(s_eg, PROV_DONE_BIT);
    return ESP_OK;
}

static void start_ap(void) {
    esp_netif_create_default_wifi_ap();
    wifi_config_t ap = {0};
    strncpy((char *)ap.ap.ssid, PROV_AP_SSID, sizeof(ap.ap.ssid));
    ap.ap.ssid_len = strlen(PROV_AP_SSID);
    ap.ap.max_connection = 4;
    ap.ap.channel = 1;
    if (strlen(PROV_AP_PASS) >= 8) {
        strncpy((char *)ap.ap.password, PROV_AP_PASS, sizeof(ap.ap.password));
        ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        ap.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "SoftAP '%s' len. Ket noi roi mo http://192.168.4.1", PROV_AP_SSID);
}

static httpd_handle_t start_http(void) {
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    if (httpd_start(&server, &cfg) == ESP_OK) {
        httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get};
        httpd_uri_t save = {.uri = "/save", .method = HTTP_POST, .handler = save_post};
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &save);
    }
    return server;
}

void wifi_provision_start_blocking(void) {
    s_eg = xEventGroupCreate();
    start_ap();
    httpd_handle_t server = start_http();
    ESP_LOGI(TAG, "Cho nguoi dung nhap WiFi...");
    xEventGroupWaitBits(s_eg, PROV_DONE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(1500));  // để response kịp gửi
    if (server) httpd_stop(server);
    esp_wifi_stop();
}
