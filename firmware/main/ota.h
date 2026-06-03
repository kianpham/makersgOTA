#ifndef OTA_H
#define OTA_H

// OTA client theo contract PHASE1.md: POST /api/v1/ota, parse activation/firmware/websocket.
#include <functional>
#include <string>

class Ota {
public:
    bool CheckVersion();  // POST /ota + parse; true nếu có websocket config

    bool HasActivationCode() const { return has_activation_code_; }
    const std::string& GetActivationCode() const { return activation_code_; }

    bool HasWebsocket() const { return has_websocket_; }
    const std::string& GetWsUrl() const { return ws_url_; }
    const std::string& GetWsToken() const { return ws_token_; }

    bool HasNewVersion() const { return has_new_version_; }
    const std::string& GetFirmwareUrl() const { return firmware_url_; }
    const std::string& GetFirmwareVersion() const { return firmware_version_; }

    // OTA tải + flash (esp_https_ota). progress: 0..100.
    bool Upgrade(const std::string& url, std::function<void(int)> progress);

    static std::string GetDeviceId();  // MAC "AA:BB:..."

private:
    bool has_activation_code_ = false;
    bool has_websocket_ = false;
    bool has_new_version_ = false;
    std::string activation_code_;
    std::string ws_url_;
    std::string ws_token_;
    std::string firmware_url_;
    std::string firmware_version_;
};

#endif  // OTA_H
