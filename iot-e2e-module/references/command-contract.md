# Hợp đồng lệnh điều khiển 3 chiều (App ↔ Cloud ↔ Firmware)

Một lệnh điều khiển tồn tại ở ba biểu diễn. Tài liệu này là nguồn sự thật chung cho cả ba mặt.

## Quy ước tên

- Lệnh: `dev.<domain>.<verb>_<noun>` — TODO(team): đổi prefix `dev.` nếu muốn.
- App (Dart) client method: `<verb><Noun>(deviceId, args)` (camelCase).
- Cloud (Python) hằng số: `CMD_<VERB>_<NOUN>` (UPPER_SNAKE); builder: `new_<verb>_<noun>_command(...)`.
- Firmware (ESP-IDF): đăng ký handler dưới đúng chuỗi tên lệnh literal.

## Bảng map kiểu (App Flutter ↔ Cloud Python ↔ Firmware ESP-IDF)

| Ý nghĩa | App (Dart) | Cloud (Python/Pydantic) | Firmware (ESP-IDF C/C++) |
|---|---|---|---|
| chuỗi | `String` | `str` | `const char*` / `std::string` |
| số nguyên | `int` | `int` | `int` / `int32_t` |
| số thực | `double` | `float` | `float` / `double` |
| boolean | `bool` | `bool` | `bool` |

> JSON number luôn là double; firmware ép về int phải kiểm range. Pydantic ở cloud validate kiểu sớm.

## Nơi mỗi mặt định nghĩa lệnh (grep pattern, không số dòng)

| Mặt | Tên lệnh | Schema argument | Thực thi/gửi |
|---|---|---|---|
| App | hằng/chuỗi trong API client (`grep` tên class client — TODO(team) đường dẫn) | model Dart `toJson` (`grep` `fromJson`) | method `dio`/`http` POST tới endpoint cloud |
| Cloud | hằng `CMD_*` (`grep "CMD_"`) | Pydantic `BaseModel` builder (`grep "class .*Args"`) | `manager.send(...)` qua WebSocket (`grep` ConnectionManager) |
| Firmware | chuỗi literal khi đăng ký handler (`grep` chỗ register command — TODO(team)) | parse `cJSON` trong handler | hành động trên thiết bị + (tùy chọn) telemetry về |

## Registry lệnh hiện có

| Tên lệnh | Argument | Mô tả |
|---|---|---|
| `dev.light.set_color` (ví dụ) | rgb: string; brightness: int 0–100=80 | Đổi màu + độ sáng đèn |
| TODO(team) | | |

## Quy tắc đồng bộ

- Sai 1 ký tự tên lệnh hoặc argument key ở bất kỳ mặt nào = lệnh rớt im lặng.
- Range: chặn UX ở app, clamp defensive ở cloud, check an toàn ở firmware — cả ba, không phụ thuộc lẫn nhau.
- Khi sửa schema một lệnh đã có: sửa đủ cả ba mặt trong cùng một lần, verify lại bảng.
