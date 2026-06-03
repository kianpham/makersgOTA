#ifndef WIFI_BOARD_H
#define WIFI_BOARD_H

// WifiBoard: provisioning SoftAP + captive portal, STA connect từ NVS (port Phase-1 C).
#include "board.h"

class WifiBoard : public Board {
public:
    WifiBoard() = default;
    std::string GetBoardType() override { return "wifi"; }

    // Blocking: nếu chưa có creds -> provisioning rồi reboot; có creds -> STA (fail -> provisioning+reboot).
    void StartNetwork() override;

    static bool HasCredentials();

    // Vào lại provisioning ở lần boot sau (đặt cờ + reboot). KHÔNG xóa WiFi đã lưu. (vd: giữ BOOT 5s)
    static void EnterConfigMode();

protected:
    void EnsureNetInit();                 // esp_netif + event loop + esp_wifi_init (1 lần)
    void StartProvisioningBlocking();     // SoftAP + HTTP portal, chặn tới khi lưu xong
    bool ConnectStaFromNvs(int timeout_ms);

    bool net_inited_ = false;
};

#endif  // WIFI_BOARD_H
