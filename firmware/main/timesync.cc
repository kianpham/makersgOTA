#include "timesync.h"

#include <cstdlib>

#include <esp_log.h>
#include <esp_netif_sntp.h>
#include <esp_sntp.h>

#define TAG "timesync"

// TZ POSIX: "ICT-7" = UTC+7 (Việt Nam), không DST.
#define TZ_STRING "ICT-7"
#define NTP_SERVER "pool.ntp.org"

static volatile bool s_synced = false;
static bool s_started = false;

static void on_sync(struct timeval* tv) {
    (void)tv;
    s_synced = true;
    ESP_LOGI(TAG, "NTP sync OK");
}

void timesync_start() {
    if (s_started) return;
    s_started = true;
    setenv("TZ", TZ_STRING, 1);
    tzset();
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
    cfg.start = true;
    cfg.sync_cb = on_sync;
    if (esp_netif_sntp_init(&cfg) != ESP_OK) {
        ESP_LOGE(TAG, "sntp init fail");
        s_started = false;
        return;
    }
    ESP_LOGI(TAG, "SNTP started (%s, TZ=%s)", NTP_SERVER, TZ_STRING);
}

bool timesync_get_local(struct tm* out) {
    if (!s_synced || !out) return false;
    time_t now = time(nullptr);
    localtime_r(&now, out);
    return true;
}
