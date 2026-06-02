#pragma once

// Kết nối WebSocket tới server (url từ OTA). Auth: header device-id + Bearer token.
// Khi connected tự gửi hello. Tự reconnect.
void ws_client_start(const char *url, const char *device_id, const char *token);
