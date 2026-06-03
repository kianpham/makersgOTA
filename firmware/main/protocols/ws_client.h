#ifndef WS_CLIENT_H
#define WS_CLIENT_H

// WebSocket client gọn (port ws_client.c) theo contract PHASE1.md: connect + gửi hello.
// Phase sau có thể nâng thành WebsocketProtocol đầy đủ của Xiaozhi (audio).
#include <functional>
#include <string>

#include <esp_websocket_client.h>

class WsClient {
public:
    void Start(const std::string& url, const std::string& device_id, const std::string& token);
    void OnConnected(std::function<void()> cb) { on_connected_ = std::move(cb); }
    void OnDisconnected(std::function<void()> cb) { on_disconnected_ = std::move(cb); }

private:
    static void EventHandler(void* arg, esp_event_base_t base, int32_t id, void* data);

    esp_websocket_client_handle_t client_ = nullptr;
    std::function<void()> on_connected_;
    std::function<void()> on_disconnected_;
};

#endif  // WS_CLIENT_H
