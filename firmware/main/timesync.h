#ifndef TIMESYNC_H
#define TIMESYNC_H

// Đồng bộ giờ SNTP + timezone POSIX. Non-blocking: gọi timesync_start() sau khi có IP.
#include <time.h>

void timesync_start();                  // cấu hình TZ + khởi động SNTP (idempotent)
bool timesync_get_local(struct tm* out);  // giờ địa phương; false nếu chưa sync

#endif  // TIMESYNC_H
