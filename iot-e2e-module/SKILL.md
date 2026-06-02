---
name: iot-e2e-module
description: Use khi cần thêm tính năng đi cả ba mặt — một lệnh/tính năng điều khiển mà app khởi phát, cloud trung chuyển, firmware thực thi (vd "thêm lệnh đổi màu đèn", "thêm nút phát âm thanh", "thêm điều khiển nhiệt độ"). Trigger khi user nói "thêm lệnh điều khiển", "tính năng đi cả app cloud firmware", "điều khiển end to end", "add e2e module" hoặc gọi `/iot-e2e-module`. Đây là skill orchestrator — gói pattern của /iot-app-module, /iot-cloud-module, /iot-fw-module và bắt buộc 3 phía khớp tên lệnh + schema. KHÔNG dùng nếu chỉ cần thêm 1 mặt.
userInvocable: true
---

# iot-e2e-module — Thêm lệnh điều khiển xuyên App + Cloud + Firmware

Skill này là **playbook end-to-end** để thêm một lệnh điều khiển đi qua cả ba mặt:

1. **App** — UI khởi phát + gọi API client.
2. **Cloud** — endpoint nhận từ app → builder + dispatch lệnh xuống thiết bị.
3. **Firmware** — handler thực thi lệnh trên thiết bị + (tùy chọn) báo trạng thái về.

Skill này KHÔNG thay thế ba skill mặt — nó **gói** cả ba + ràng buộc đồng bộ tên lệnh và schema ở chỗ ba mặt gặp nhau.

> Quy ước tên lệnh: `dev.<domain>.<verb>_<noun>`. TODO(team): đổi prefix `dev.` thành miền của bạn.

---

## Bộ ràng buộc đồng bộ 3 chiều (đọc trước khi code)

Một lệnh điều khiển tồn tại ở **ba biểu diễn** phải khớp tuyệt đối. Sai một chỗ = lệnh rớt im lặng, runtime error khó debug.

| Thành phần | App | Cloud | Firmware |
|---|---|---|---|
| **Tên lệnh** | hằng/chuỗi trong API client | hằng `Cmd<Name>` | chuỗi literal khi đăng ký handler |
| **Tên argument** | key trong payload gửi đi | key trong builder | key khi parse trong handler |
| **Kiểu argument** | kiểu của app | kiểu của cloud | kiểu của firmware |
| **Required/optional + default** | form/validation app | builder truyền giá trị | khai báo required vs default |
| **Range/validation** | chặn ở UI (UX) | clamp ở cloud (defensive) | check ở firmware (an toàn) |

**Luôn dán ba đoạn cạnh nhau khi xong để xác minh.** Bảng map kiểu (App Flutter ↔ Cloud Python ↔ Firmware ESP-IDF):

| Ý nghĩa | App (Dart) | Cloud (Python/Pydantic) | Firmware (ESP-IDF C/C++) |
|---|---|---|---|
| chuỗi | `String` | `str` | `const char*` / `std::string` (`cJSON_GetStringValue`) |
| số nguyên | `int` | `int` | `int` / `int32_t` (`cJSON_GetNumberValue` → ép int) |
| số thực | `double` | `float` | `float` / `double` |
| boolean | `bool` | `bool` | `bool` (`cJSON_IsTrue`) |

> Lưu ý JSON: số trong JSON là `double`; firmware ép về `int` phải kiểm tràn/range. Cloud (Pydantic) validate kiểu tự động — tận dụng để chặn sai kiểu sớm.

---

## Workflow chuẩn

```
Bước 1 — Thiết kế lệnh
  - Đặt tên: dev.<domain>.<verb>_<noun>
  - Liệt kê argument + kiểu + required/optional + default + range
  - Mô tả 1 câu

Bước 2 — Firmware (theo /iot-fw-module mục "command handler")
  - Đăng ký handler thực thi lệnh
  - Format + build

Bước 3 — Cloud (theo /iot-cloud-module mục "lệnh xuống thiết bị")
  - Hằng số tên lệnh + builder + dispatch helper
  - Endpoint cho app gọi tới (mục REST của /iot-cloud-module)
  - Lint + build + test

Bước 4 — App (theo /iot-app-module mục "action điều khiển")
  - API client method + UI element gọi nó
  - Phản ánh trạng thái thật, không giả định thành công

Bước 5 — VERIFY đồng bộ 3 chiều
  - Dán 3 đoạn (App, Cloud, FW) cạnh nhau
  - So: tên lệnh, mỗi argument key, kiểu, default
  - Lệch → STOP, fix trước khi commit

Bước 6 — Integration test
  - Chạy cloud local, flash firmware trỏ tới cloud, mở app
  - Bấm nút trên app → quan sát log cloud "gửi lệnh" → log firmware "nhận lệnh" → hành vi thiết bị → app cập nhật trạng thái
```

---

## Ví dụ end-to-end: `dev.light.set_color`

Yêu cầu: app có nút chọn màu → cloud gửi lệnh → đèn LED đổi màu.
Argument: `rgb` (string hex, bắt buộc); `brightness` (int 0–100, default 80).

**Bước 2 — Firmware:** đăng ký handler tên `"dev.light.set_color"`, parse `rgb` + `brightness`, set LED. (Chi tiết: `/iot-fw-module` mục 2.)

**Bước 3 — Cloud:**
```
Cmd SetColor = "dev.light.set_color"

New SetColorCommand(id, rgb, brightness):
    { id, name: CmdSetColor, args: { rgb, brightness } }

helper SetColor(deviceId, rgb, brightness):
    clamp brightness to [0,100]
    dispatch to device(New SetColorCommand(nextID(), rgb, brightness))

POST /devices/:id/light/color  → gọi SetColor(...)
```

**Bước 4 — App:**
```
client.setLightColor(deviceId, { rgb, brightness })  → POST /devices/:id/light/color
UI: color picker + slider; sau khi gửi, chờ realtime state xác nhận màu mới
```

**Bước 5 — Verify:**

| Item | App | Cloud | FW | Khớp? |
|---|---|---|---|---|
| Tên lệnh | "dev.light.set_color" | CmdSetColor = "dev.light.set_color" | đăng ký "dev.light.set_color" | ✓ |
| Arg `rgb` | string (required) | string | string (required) | ✓ |
| Arg `brightness` | int (default 80) | int (clamp 0–100) | int (default 80, [0,100]) | ✓ |

---

## Build + test cuối cùng

```bash
# Firmware (ESP-IDF):
idf.py clang-format && idf.py build flash monitor
# Cloud (Python/FastAPI):
ruff check . && black . && pytest && uvicorn app.main:app --reload   # TODO(team): entrypoint thật
# App (Flutter):
dart format lib && flutter analyze && flutter test && flutter run
# Trigger: bấm nút app → log cloud "gửi lệnh" → log firmware "nhận lệnh" → hành vi thiết bị → app cập nhật trạng thái
```

---

## Khi nào KHÔNG dùng skill này

| Tình huống | Dùng thay |
|---|---|
| Chỉ thêm phía firmware | `/iot-fw-module` |
| Chỉ thêm builder cloud cho lệnh đã có | `/iot-cloud-module` |
| Chỉ thêm UI/API client | `/iot-app-module` |

---

## Checklist trước khi báo xong

- [ ] Tên lệnh **trùng tuyệt đối** ở cả 3 mặt
- [ ] Mọi argument key **trùng tuyệt đối** ở cả 3 mặt
- [ ] Kiểu khớp theo bảng map (App ↔ Cloud ↔ FW)
- [ ] Default/clamp nhất quán 3 phía
- [ ] Mỗi mặt: format/lint + build + test xanh
- [ ] Integration: log cloud + log firmware + hành vi thiết bị + app cập nhật đều đúng
- [ ] TODO(team): cập nhật bảng registry lệnh (references) nếu thêm lệnh mới
- [ ] Báo cáo: 3 đoạn patch cạnh nhau, log mẫu, cách trigger test

---

## Tham khảo

- [references/command-contract.md](references/command-contract.md) — đặc tả hợp đồng 3 chiều
- [/iot-fw-module](../iot-fw-module/SKILL.md), [/iot-cloud-module](../iot-cloud-module/SKILL.md), [/iot-app-module](../iot-app-module/SKILL.md)
