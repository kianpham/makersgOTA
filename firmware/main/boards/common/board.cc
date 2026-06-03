#include "board.h"

#include <esp_random.h>
#include <esp_log.h>

#include "display.h"
#include "settings.h"

#define TAG "Board"

Board::Board() {
    // UUID ổn định lưu NVS (tái dùng pattern Xiaozhi).
    Settings settings("board", true);
    uuid_ = settings.GetString("uuid");
    if (uuid_.empty()) {
        uuid_ = GenerateUuid();
        settings.SetString("uuid", uuid_);
    }
    ESP_LOGI(TAG, "UUID=%s", uuid_.c_str());
}

std::string Board::GenerateUuid() {
    uint8_t uuid[16];
    esp_fill_random(uuid, sizeof(uuid));
    uuid[6] = (uuid[6] & 0x0F) | 0x40;  // version 4
    uuid[8] = (uuid[8] & 0x3F) | 0x80;  // variant 1
    char s[37];
    snprintf(s, sizeof(s), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8],
             uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
    return std::string(s);
}

Display* Board::GetDisplay() {
    static NoDisplay display;
    return &display;
}
