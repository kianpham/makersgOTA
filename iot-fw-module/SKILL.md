---
name: iot-fw-module
description: Use khi cần thêm module mới vào firmware thiết bị của <sản phẩm> (firmware/). Bao gồm driver/peripheral mới (sensor, actuator, display), command handler thực thi lệnh từ cloud, telemetry/event gửi lên cloud, hoặc lớp comms/protocol mới. Trigger khi user nói "thêm driver", "thêm sensor", "thêm command handler thiết bị", "thêm telemetry", "thêm protocol firmware", "viết module firmware", "add fw module" hoặc gọi `/iot-fw-module`. KHÔNG dùng cho bring-up board mới (việc một lần, copy thư mục board mẫu). KHÔNG dùng khi tính năng đi cả cloud+app — dùng /iot-e2e-module.
userInvocable: true
---

# iot-fw-module — Thêm module firmware (<sản phẩm>)

Skill này thêm một implementation mới vào firmware thiết bị. Triết lý: **minimum touch** — thêm file mới kế thừa interface có sẵn, chỉ chạm file lõi tối thiểu để đăng ký.

> Repo: `firmware/` — Board **LCDWIKI ES3C35P**: ESP32-S3 (target `esp32s3`), màn ST77922 QSPI 320×480, 8MB OPI PSRAM, 16MB flash.
> Build framework: **ESP-IDF** (C/C++, component-based, CMake). Pin-map đầy đủ: [references/board-es3c35p.md](references/board-es3c35p.md).

---

## Khi nào KHÔNG dùng skill này

| Tình huống | Dùng thay |
|---|---|
| Bring-up **board/biến thể phần cứng** mới | Việc một lần, không skill: `idf.py create-project` → `set-target esp32s3` → cấu hình PSRAM OPI + flash 16MB trong `menuconfig`, copy ví dụ LCDWIKI làm tham chiếu. |
| Tính năng đi cả cloud + app (lệnh điều khiển mới end-to-end) | `/iot-e2e-module` |
| Chỉ thêm builder phía cloud cho lệnh firmware đã có | `/iot-cloud-module` |
| Fix bug / refactor file đã có | Không cần skill — sửa trực tiếp |

---

## Quy tắc bất di bất dịch

1. **Không sửa interface/abstract base.** Chỉ thêm file mới kế thừa.
2. **Tối thiểu chạm file lõi.** Lý tưởng: một `#include` + một `register/factory`. Nếu phải sửa nhiều hơn, dừng và hỏi user.
3. **Build manifest (ESP-IDF)** — thêm file `.c`/`.cpp` mới vào `SRCS` trong `idf_component_register(...)` của `CMakeLists.txt` thuộc component tương ứng (grep `idf_component_register`). Chỉ thêm, không xóa/reorder. Nếu là component mới, khai báo `REQUIRES`/`PRIV_REQUIRES` đúng dependency.
4. **Ràng buộc tài nguyên (board ES3C35P):** buffer lớn (>4KB: framebuffer, ảnh, audio) xin từ PSRAM — `heap_caps_malloc(.., MALLOC_CAP_SPIRAM)` fallback `MALLOC_CAP_8BIT`. Board có 8MB OPI PSRAM. GPIO trống chỉ còn IO45/IO46. Một bus I2C dùng chung (IO38/IO39) cho touch + codec + mở rộng — đừng tạo bus thứ hai.
5. **Code style: clang-format.** Chạy `idf.py clang-format` (hoặc `clang-format -i <file>` theo `.clang-format` của repo) trước khi báo xong.
6. **Tham chiếu file mẫu bằng pattern `grep`**, không bằng số dòng — code sẽ đổi.
7. Đọc file lớn theo range, đừng cat toàn bộ.

---

## Workflow chung

```
1. Xác định loại module (driver / command handler / telemetry / comms)
2. Đọc 1 implementation gần nhất với case đang làm (tương đồng nhất, không phải phức tạp nhất)
3. Đọc interface header → liệt kê hàm bắt buộc override
4. Tạo file mới trong đúng thư mục
5. Thêm vào build manifest
6. Wiring: grep chỗ đăng ký/instantiate hiện tại, thêm nhánh mới
7. Format
8. Build
9. Báo cáo: file tạo, dòng sửa lõi, lệnh build, cách kiểm runtime
```

---

## 1. Thêm driver / peripheral (sensor, actuator, display)

TODO(team): điền interface thật. Khung chung:

1. `firmware/<dir>/my_driver.{h,impl}` — kế thừa base, khai báo handle phần cứng (I2C/SPI/GPIO).
2. Constructor: init bus + cấu hình chip. Override hàm đọc/ghi.
3. Thêm vào build manifest.
4. Wiring: trong file board của bạn, override getter trả về instance — `grep` pattern: TODO(team) (vd `GetAudioCodec`, `GetDisplay`).

## 2. Thêm command handler (cloud → thiết bị)

Đây là nửa firmware của một lệnh điều khiển. **Nếu cloud + app cũng cần đổi, dùng `/iot-e2e-module` thay vì skill này.**

1. Trong file đăng ký lệnh — `grep` pattern: TODO(team) (vd `RegisterCommands`, `AddTool`) — thêm một block đăng ký:
   - Tên lệnh: `dev.<domain>.<verb>_<noun>` (khớp tuyệt đối với cloud).
   - Khai báo argument + kiểu + required/default.
   - Lambda/handler đọc argument rồi thực thi hành động trên thiết bị.
2. Logic dài → tách helper riêng cùng file, lambda chỉ gọi helper.
3. TODO(team): network/file phải có timeout; lỗi cần báo về cloud → throw/return error đúng cách.

## 3. Thêm telemetry / event (thiết bị → cloud)

1. Thu thập dữ liệu (đọc sensor, đếm sự kiện).
2. Đóng gói message theo schema chung — `grep` pattern struct message: TODO(team).
3. Gửi qua lớp comms (xem mục 4) — đừng tự mở socket.

## 4. Thêm lớp comms / protocol mới

1. Kế thừa interface protocol — `grep`: TODO(team).
2. Override lifecycle (connect, mở kênh, gửi/nhận), gọi callback khi có sự kiện.
3. Wiring: chọn protocol qua config — `grep` chỗ instantiate.

---

## Build & test

```bash
# Target chip cố định: esp32s3
idf.py set-target esp32s3          # chỉ lần đầu / khi đổi target
idf.py clang-format                # format trước khi báo xong
idf.py build                       # biên dịch — bắt undefined reference sớm
idf.py flash monitor               # nạp + xem log đăng ký module mới; Ctrl+] để thoát
#   Lưu ý: backlight (IO41) phải được kéo cao trong init nếu không màn đen.
#   Unit test on-host (tùy chọn): component test với Unity dưới test/ — TODO(team) nếu dùng.
```

Lưu ý board: chọn đúng driver **ST77922 QSPI** cho màn; init phải kéo cao **IO41** (backlight) nếu không màn đen.
Lỗi `undefined reference` → quên thêm vào build manifest.

---

## Checklist trước khi báo xong

- [ ] File mới đã format
- [ ] Thêm vào build manifest (kiểm bằng `grep <tên file> <manifest>`)
- [ ] Build xanh (báo command + output cuối)
- [ ] Không sửa interface/abstract base
- [ ] Nếu là command handler đi cả cloud: tên lệnh + schema khớp — nên chuyển sang `/iot-e2e-module`
- [ ] Báo cáo: file tạo mới, dòng sửa lõi (tối thiểu), lệnh build/flash, cách kiểm runtime

---

## Tham khảo

- [references/board-es3c35p.md](references/board-es3c35p.md) — **pin-map + thông số mạch đầy đủ (board ES3C35P)**
- [references/fw-interfaces.md](references/fw-interfaces.md) — chi tiết interface từng base class (TODO(team) điền)
