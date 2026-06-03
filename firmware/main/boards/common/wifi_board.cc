// WifiBoard mạng — port từ wifi_provision.c + wifi_sta.c (Phase 1) sang C++.
#include "wifi_board.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <nvs.h>

#include "app_config.h"

#define TAG "WifiBoard"

// ---- Provisioning state ----
static EventGroupHandle_t s_prov_eg;
#define PROV_DONE_BIT BIT0

// ---- STA state ----
static EventGroupHandle_t s_sta_eg;
#define STA_CONNECTED_BIT BIT0
#define STA_FAIL_BIT BIT1
static int s_sta_retry;
#define STA_MAX_RETRY 5

void WifiBoard::EnsureNetInit() {
    if (net_inited_) return;
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wcfg));
    net_inited_ = true;
}

// Cờ NVS "force_prov" (namespace "sys"): yêu cầu vào provisioning ở lần boot sau (KHÔNG xóa WiFi cũ).
static bool prov_requested() {
    nvs_handle_t h;
    bool v = false;
    if (nvs_open("sys", NVS_READONLY, &h) == ESP_OK) {
        uint8_t x = 0;
        if (nvs_get_u8(h, "force_prov", &x) == ESP_OK) v = (x != 0);
        nvs_close(h);
    }
    return v;
}

static void set_prov_request(bool on) {
    nvs_handle_t h;
    if (nvs_open("sys", NVS_READWRITE, &h) == ESP_OK) {
        if (on)
            nvs_set_u8(h, "force_prov", 1);
        else
            nvs_erase_key(h, "force_prov");
        nvs_commit(h);
        nvs_close(h);
    }
}

void WifiBoard::EnterConfigMode() {
    // KHÔNG xóa WiFi đã lưu — chỉ đặt cờ rồi reboot vào provisioning.
    set_prov_request(true);
    ESP_LOGW(TAG, "Yeu cau cau hinh WiFi -> reboot (giu WiFi cu)");
    esp_restart();
}

bool WifiBoard::HasCredentials() {
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READONLY, &h) != ESP_OK) return false;
    size_t len = 0;
    bool ok = (nvs_get_str(h, "ssid", nullptr, &len) == ESP_OK) && len > 1;
    nvs_close(h);
    return ok;
}

// ---------- Provisioning (SoftAP + HTTP portal) ----------
// Trang captive portal — HTML/CSS responsive (mọi thiết bị), quét + chọn mạng WiFi.
static const char* PROV_PAGE = R"HTML(<!doctype html>
<html lang="vi"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>MakerSG WiFi</title>
<style>
:root{--bg:#000;--card:#111827;--ac:#1A6FF5;--fg:#f1f5f9;--mut:#94a3b8;--in:#0b1220;--bd:#334155}
*{box-sizing:border-box}
body{margin:0;font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;background:var(--bg);color:var(--fg);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:16px}
.card{width:100%;max-width:420px;background:var(--card);border-radius:18px;padding:24px;box-shadow:0 12px 40px rgba(0,0,0,.6)}
.hd{display:flex;align-items:baseline;justify-content:space-between}
h1{margin:0;font-size:24px;font-weight:800;letter-spacing:2px}
.sub{color:var(--mut);font-size:13px;margin:2px 0 18px}
label{display:block;font-size:12px;color:var(--mut);margin:16px 0 6px;text-transform:uppercase;letter-spacing:.5px}
input{width:100%;padding:13px 14px;border-radius:11px;border:1px solid var(--bd);background:var(--in);color:var(--fg);font-size:16px}
input:focus{outline:none;border-color:var(--ac)}
.pw{position:relative}
.tg{position:absolute;right:8px;top:50%;transform:translateY(-50%);background:none;border:none;color:var(--mut);font-size:13px;padding:6px 8px;cursor:pointer}
.btn{width:100%;margin-top:22px;padding:14px;border:none;border-radius:11px;background:var(--ac);color:#fff;font-size:16px;font-weight:700;cursor:pointer}
.btn:active{opacity:.85}
.scan{background:none;border:1px solid var(--bd);color:var(--fg);padding:7px 12px;border-radius:9px;font-size:13px;cursor:pointer}
.nets{margin-top:6px;max-height:190px;overflow:auto}
.net{display:flex;justify-content:space-between;align-items:center;gap:8px;padding:12px 14px;background:var(--in);border:1px solid #1f2937;border-radius:11px;margin-bottom:7px;cursor:pointer}
.net:active,.net.sel{border-color:var(--ac)}
.net .n{font-size:15px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.net .s{font-size:12px;color:var(--mut);flex:none}
.msg{margin-top:14px;font-size:12px;color:var(--mut);text-align:center}
</style></head>
<body><div class="card">
<div class="hd"><h1>MAKER SG</h1><button class="scan" onclick="scan()">Quet lai</button></div>
<div class="sub">Ket noi WiFi cho thiet bi</div>
<label>Mang kha dung</label>
<div class="nets" id="nets"><div class="msg">Dang quet...</div></div>
<form method="POST" action="/save">
<label>Ten WiFi (SSID)</label>
<input name="ssid" id="ssid" autocomplete="off" required>
<label>Mat khau</label>
<div class="pw"><input name="pass" id="pass" type="password" autocomplete="off"><button type="button" class="tg" onclick="tp(this)">Hien</button></div>
<button class="btn" type="submit">Luu &amp; Ket noi</button>
</form>
<div class="msg">Thiet bi se khoi dong lai sau khi luu.</div>
</div>
<script>
function tp(b){var p=document.getElementById('pass');if(p.type=='password'){p.type='text';b.textContent='An'}else{p.type='password';b.textContent='Hien'}}
function pick(el,s){document.getElementById('ssid').value=s;document.querySelectorAll('.net').forEach(function(n){n.classList.remove('sel')});el.classList.add('sel');document.getElementById('pass').focus()}
function scan(){var n=document.getElementById('nets');n.innerHTML='<div class=msg>Dang quet...</div>';
fetch('/scan').then(function(r){return r.json()}).then(function(l){if(!l.length){n.innerHTML='<div class=msg>Khong thay mang nao, nhap tay ben duoi</div>';return}n.innerHTML='';l.forEach(function(w){var d=document.createElement('div');d.className='net';d.onclick=function(){pick(d,w.ssid)};var lk=w.lock?'&#128274; ':'';d.innerHTML='<span class=n>'+w.ssid+'</span><span class=s>'+lk+w.rssi+'dBm</span>';n.appendChild(d)})}).catch(function(){n.innerHTML='<div class=msg>Quet loi, nhap tay ben duoi</div>'})}
scan();
</script>
</body></html>)HTML";

static esp_err_t root_get(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, PROV_PAGE, HTTPD_RESP_USE_STRLEN);
}

// Escape chuỗi cho JSON (chỉ " và \\; bỏ ký tự điều khiển).
static void json_escape(const char* in, char* out, size_t outsz) {
    size_t o = 0;
    for (size_t i = 0; in[i] && o + 2 < outsz; i++) {
        unsigned char c = (unsigned char)in[i];
        if (c == '"' || c == '\\') {
            out[o++] = '\\';
            out[o++] = (char)c;
        } else if (c >= 0x20) {
            out[o++] = (char)c;
        }
    }
    out[o] = 0;
}

// GET /scan -> JSON [{ssid,rssi,lock}] các mạng WiFi quét được (cần mode APSTA).
static esp_err_t scan_get(httpd_req_t* req) {
    wifi_scan_config_t sc = {};
    httpd_resp_set_type(req, "application/json");
    if (esp_wifi_scan_start(&sc, true) != ESP_OK) {
        return httpd_resp_sendstr(req, "[]");
    }
    uint16_t n = 0;
    esp_wifi_scan_get_ap_num(&n);
    if (n > 24) n = 24;
    wifi_ap_record_t* recs = n ? (wifi_ap_record_t*)calloc(n, sizeof(wifi_ap_record_t)) : nullptr;
    if (recs) esp_wifi_scan_get_ap_records(&n, recs);

    char* json = (char*)malloc(4096);
    int off = 0;
    json[off++] = '[';
    char seen[24][33];
    int seenc = 0;
    for (int i = 0; recs && i < n; i++) {
        const char* ssid = (const char*)recs[i].ssid;
        if (ssid[0] == 0) continue;
        bool dup = false;
        for (int j = 0; j < seenc; j++)
            if (strcmp(seen[j], ssid) == 0) {
                dup = true;
                break;
            }
        if (dup) continue;
        if (seenc < 24) {
            strncpy(seen[seenc], ssid, 32);
            seen[seenc][32] = 0;
            seenc++;
        }
        char esc[80];
        json_escape(ssid, esc, sizeof(esc));
        off += snprintf(json + off, 4096 - off, "%s{\"ssid\":\"%s\",\"rssi\":%d,\"lock\":%d}",
                        off > 1 ? "," : "", esc, recs[i].rssi,
                        recs[i].authmode != WIFI_AUTH_OPEN ? 1 : 0);
        if (off > 3900) break;
    }
    json[off++] = ']';
    json[off] = 0;
    esp_err_t r = httpd_resp_send(req, json, off);
    free(recs);
    free(json);
    return r;
}

// Giải mã URL (form x-www-form-urlencoded): '+' -> ' ', '%XX' -> byte. Sửa tại chỗ.
static void url_decode(char* s) {
    auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        c = (char)(c | 0x20);
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return 0;
    };
    char* o = s;
    for (char* p = s; *p; ++p) {
        if (*p == '+') {
            *o++ = ' ';
        } else if (*p == '%' && p[1] && p[2]) {
            *o++ = (char)(hex(p[1]) * 16 + hex(p[2]));
            p += 2;
        } else {
            *o++ = *p;
        }
    }
    *o = 0;
}

static esp_err_t save_post(httpd_req_t* req) {
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
    url_decode(ssid);  // giải mã URL (mật khẩu có ký tự đặc biệt/dấu cách bị mã hóa)
    url_decode(pass);
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
    ESP_LOGI(TAG, "Da luu SSID='%s' (pass len=%d)", ssid, (int)strlen(pass));

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, "<h3>Da luu. Thiet bi se khoi dong lai va ket noi.</h3>",
                    HTTPD_RESP_USE_STRLEN);
    xEventGroupSetBits(s_prov_eg, PROV_DONE_BIT);
    return ESP_OK;
}

void WifiBoard::StartProvisioningBlocking() {
    s_prov_eg = xEventGroupCreate();
    esp_netif_create_default_wifi_ap();
    wifi_config_t ap = {};
    strncpy((char*)ap.ap.ssid, PROV_AP_SSID, sizeof(ap.ap.ssid));
    ap.ap.ssid_len = strlen(PROV_AP_SSID);
    ap.ap.max_connection = 4;
    ap.ap.channel = 1;
    if (strlen(PROV_AP_PASS) >= 8) {
        strncpy((char*)ap.ap.password, PROV_AP_PASS, sizeof(ap.ap.password));
        ap.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        ap.ap.authmode = WIFI_AUTH_OPEN;
    }
    // APSTA: AP phục vụ portal + STA bật để quét được danh sách WiFi.
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "SoftAP '%s' -> http://192.168.4.1", PROV_AP_SSID);

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = nullptr;
    if (httpd_start(&server, &cfg) == ESP_OK) {
        httpd_uri_t root = {.uri = "/", .method = HTTP_GET, .handler = root_get, .user_ctx = nullptr};
        httpd_uri_t scan = {.uri = "/scan", .method = HTTP_GET, .handler = scan_get, .user_ctx = nullptr};
        httpd_uri_t save = {.uri = "/save", .method = HTTP_POST, .handler = save_post, .user_ctx = nullptr};
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &scan);
        httpd_register_uri_handler(server, &save);
    }
    ESP_LOGI(TAG, "Cho nguoi dung nhap WiFi...");
    xEventGroupWaitBits(s_prov_eg, PROV_DONE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(1500));
    if (server) httpd_stop(server);
    esp_wifi_stop();
}

// ---------- STA connect ----------
static void sta_evt(void* arg, esp_event_base_t base, int32_t id, void* data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        auto* d = static_cast<wifi_event_sta_disconnected_t*>(data);
        // reason: 15=4WAY_HANDSHAKE_TIMEOUT/sai pass, 201=NO_AP_FOUND, 2=AUTH_EXPIRE, 205=CONNECTION_FAIL
        ESP_LOGW(TAG, "STA disconnected reason=%d (retry %d/%d)", d ? d->reason : -1, s_sta_retry,
                 STA_MAX_RETRY);
        if (s_sta_retry++ < STA_MAX_RETRY) {
            esp_wifi_connect();
        } else {
            xEventGroupSetBits(s_sta_eg, STA_FAIL_BIT);
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        s_sta_retry = 0;
        xEventGroupSetBits(s_sta_eg, STA_CONNECTED_BIT);
    }
}

bool WifiBoard::ConnectStaFromNvs(int timeout_ms) {
    char ssid[64] = {0}, pass[64] = {0};
    size_t sl = sizeof(ssid), pl = sizeof(pass);
    nvs_handle_t h;
    if (nvs_open("wifi", NVS_READONLY, &h) != ESP_OK) return false;
    esp_err_t e1 = nvs_get_str(h, "ssid", ssid, &sl);
    nvs_get_str(h, "pass", pass, &pl);
    nvs_close(h);
    if (e1 != ESP_OK) return false;

    s_sta_eg = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &sta_evt, nullptr, nullptr);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &sta_evt, nullptr, nullptr);

    wifi_config_t sta = {};
    strncpy((char*)sta.sta.ssid, ssid, sizeof(sta.sta.ssid));
    strncpy((char*)sta.sta.password, pass, sizeof(sta.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_sta_eg, STA_CONNECTED_BIT | STA_FAIL_BIT, pdFALSE,
                                           pdFALSE, pdMS_TO_TICKS(timeout_ms));
    bool ok = (bits & STA_CONNECTED_BIT) != 0;
    ESP_LOGI(TAG, "%s (SSID=%s)", ok ? "STA connected" : "STA failed/timeout", ssid);
    return ok;
}

void WifiBoard::StartNetwork() {
    EnsureNetInit();
    bool force = prov_requested();
    if (force) set_prov_request(false);  // dùng 1 lần -> xóa cờ ngay
    if (force || !HasCredentials()) {
        ESP_LOGI(TAG, "%s", force ? "Cau hinh WiFi theo yeu cau (giu WiFi cu)"
                                  : "Chua co WiFi -> provisioning");
        StartProvisioningBlocking();
        ESP_LOGI(TAG, "Da luu WiFi -> restart");
        esp_restart();
    }
    if (!ConnectStaFromNvs(WIFI_STA_TIMEOUT_MS)) {
        ESP_LOGW(TAG, "Khong ket noi WiFi -> quay lai provisioning");
        StartProvisioningBlocking();
        esp_restart();
    }
}
