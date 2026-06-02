# QUICKSTART — MakerSG (IoT trên board ES3C35P)

Đưa một người mới từ zero đến chạy được tính năng điều khiển đầu tiên (app → cloud → thiết bị).

> Phần cứng: LCDWIKI **ES3C35P** — ESP32-S3, màn ST77922 QSPI 320×480, audio (mic + loa + codec), LED RGB, thẻ SD, pin LiPo.
> Cách làm việc: dùng bộ skill 3 mặt + 1 orchestrator để agent (Claude Code) thêm tính năng theo recipe, giữ 3 phía đồng bộ.

---

## 0. Toàn cảnh dự án

Sản phẩm có **ba mặt**, một tính năng điển hình đụng cả ba:

```
App điều khiển  ──HTTP/API──▶  Cloud/Backend  ──lệnh qua dây──▶  Firmware (ES3C35P)
   (UI, nút bấm)                (endpoint + builder lệnh)         (handler thực thi)
        ▲                              │                                  │
        └──────── realtime trạng thái ◀┴──────── telemetry/event ◀────────┘
```

Mỗi mặt có một skill; tính năng xuyên 3 mặt đi qua orchestrator:

| Mặt | Skill | Khi dùng |
|---|---|---|
| Firmware | `/iot-fw-module` | driver, peripheral, handler lệnh, telemetry, comms |
| Cloud | `/iot-cloud-module` | endpoint, builder lệnh, handler event, data model |
| App | `/iot-app-module` | màn hình, API client, action điều khiển, realtime |
| Xuyên 3 mặt | `/iot-e2e-module` | **lệnh điều khiển mới end-to-end** (dùng nhiều nhất) |

---

## 1. Cần chuẩn bị

| Thứ | Ghi chú |
|---|---|
| Phần cứng | Board ES3C35P + cáp Type-C (có sẵn trong hộp) |
| Claude Code | Để agent dùng skill. Cài: `npm i -g @anthropic-ai/claude-code` |
| Toolchain firmware | **ESP-IDF** (target chip cố định `esp32s3`). Cài theo hướng dẫn Espressif; dùng `idf.py`. |
| Toolchain cloud | **Python 3.11+** + FastAPI + asyncio. `pip install fastapi uvicorn pydantic`; lint `ruff`+`black`, test `pytest`. |
| Toolchain app | **Flutter SDK** (Dart). `flutter doctor` để kiểm môi trường; build mobile iOS/Android. |
| Driver USB | Win10+ hỗ trợ USB virtual serial sẵn; bản cũ hơn cần xử lý thêm |

---

## 2. Cài đặt (một lần)

```bash
# 1. Clone repo dự án
git clone <REPO_URL>            # TODO(team)
cd <REPO>

# 2. Cài bộ skill vào project (agent tự nhận)
mkdir -p .claude/skills
cp -r iot-skills/iot-* .claude/skills/
cp iot-skills/CLAUDE.md ./CLAUDE.md     # hoặc merge vào CLAUDE.md sẵn có

# 3. Xem các chỗ còn cần điền
grep -rn "TODO(team)" .claude/skills CLAUDE.md
```

> **Windows / PowerShell** (lệnh bash ở trên không chạy được — dùng bản này):
>
> ```powershell
> New-Item -ItemType Directory -Force .claude\skills | Out-Null
> Copy-Item -Recurse iot-skills\iot-* .claude\skills\
> Copy-Item iot-skills\CLAUDE.md .\CLAUDE.md
> Select-String -Path .claude\skills\*\*.md, CLAUDE.md -Pattern "TODO\(team\)"
> ```

Mở Claude Code trong repo, gõ `/` để xác nhận có `/iot-fw-module`, `/iot-cloud-module`, `/iot-app-module`, `/iot-e2e-module`.

---

## 3. Bring-up phần cứng (kiểm tra board sống)

### Vào chế độ nạp
Giữ **BOOT**, cấp nguồn (cắm Type-C), rồi thả BOOT.
Hoặc khi đã có nguồn: giữ BOOT → nhấn RESET → thả RESET → thả BOOT.

### Nạp ví dụ chuẩn trước khi tự viết
Dùng chương trình mẫu LCDWIKI **không sửa gì** để xác nhận màn + cảm ứng + audio chạy, loại trừ lỗi phần cứng.

```bash
# ESP-IDF:
idf.py set-target esp32s3
idf.py build flash monitor      # Ctrl+] để thoát monitor
```

### Bẫy phải biết (từ FAQ spec)
- **Màn đen** dù đã nạp: code init **bắt buộc kéo cao IO41** (backlight) — không bật là không sáng.
- **Backlight sáng nhưng không hiển thị**: kiểm tra chọn đúng driver **ST77922** + định nghĩa GPIO QSPI.
- Dùng **PSRAM** cho framebuffer/buffer lớn (board có 8MB OPI PSRAM).

---

## 4. Tính năng đầu tiên — ví dụ "đổi màu LED RGB" (end-to-end)

LED RGB nằm trên **IO40** (địa chỉ hóa, kiểu WS2812). Đây là "hello world" tự nhiên cho board này.

Trong Claude Code, gọi orchestrator:

```
/iot-e2e-module thêm lệnh dev.light.set_color.
- Args: rgb (string hex, bắt buộc); brightness (int 0–100, default 80)
- App: nút color picker → client.setLightColor(deviceId, {rgb, brightness})
- Cloud: endpoint POST /devices/:id/light/color → builder + dispatch
- Firmware: handler set LED RGB trên IO40 theo rgb + brightness
```

Skill sẽ dẫn qua 6 bước: thiết kế → firmware → cloud → app → **verify đồng bộ 3 chiều** → integration test. Bước verify ép dán 3 đoạn cạnh nhau để chắc **tên lệnh + tên/kiểu argument khớp tuyệt đối** — sai 1 ký tự là lệnh rớt im lặng.

### Quy ước hợp đồng (nhớ kỹ)
- Tên lệnh: `dev.<domain>.<verb>_<noun>` (TODO(team): đổi prefix `dev.`).
- Range chặn 3 lớp độc lập: UX ở app, clamp ở cloud, an toàn ở firmware.
- App **không** giả định lệnh thành công — phản ánh trạng thái thật khi nhận realtime.

---

## 5. Vòng lặp hằng ngày

1. Xác định tính năng đụng mấy mặt → chọn skill (xem bảng mục 0).
2. Gọi skill, mô tả tính năng theo đúng các trường nó cần (tên, args, type, required/default).
3. Để skill chạy recipe + checklist của nó.
4. Tính năng xuyên 3 mặt → luôn qua `/iot-e2e-module` để hợp đồng được gác.
5. Bring-up board mới / setup hạ tầng một lần → **không dùng skill** (xem CLAUDE.md).

---

## 6. Khắc phục sự cố (rút từ FAQ spec)

| Triệu chứng | Hướng kiểm tra |
|---|---|
| Nạp xong, màn không sáng | Code chưa bật backlight (IO41); kiểm nguồn/cáp/pin |
| Backlight sáng, không hiển thị | Sai driver (phải ST77922) hoặc GPIO QSPI; nạp ví dụ chuẩn trước |
| Hiển thị OK, chạm không ăn | Code thiếu chức năng touch; GPIO touch sai; hỏng IC/cáp touch |
| Backlight chập chờn | Mạch backlight dùng FET + PWM trên pin điều khiển; nguồn không ổn định |
| Pin ngoài không sạc | Chỉ hỗ trợ LiPo 3.7V; sai cực; pin đã đầy; hỏng IC sạc |
| Nạp chương trình thất bại | Vào lại download mode (BOOT+nguồn / BOOT+RESET); Win10 USB virtual serial; sai/đụng cổng COM |
| Cắm loa không có tiếng | Sai chương trình; loa tiếp xúc kém/hỏng; loa quá công suất (max 1.5W@8Ω hoặc 2W@4Ω) |

---

## 7. Còn phải điền (TODO(team))

- ~~Build framework firmware~~ → **ESP-IDF** (đã chốt). Lệnh build/format: `idf.py build` / `idf.py clang-format`.
- ~~Stack cloud + app~~ → **Cloud Python/FastAPI**, **App Flutter** (đã chốt). Còn lại: đường dẫn repo thật, DB cloud, state lib app.
- Interface phần mềm firmware (`iot-fw-module/references/fw-interfaces.md`) — đã có khung ESP-IDF, cần trỏ grep pattern tới code thật khi có.
- Xác nhận từ schematic: IC cảm ứng (I2C 0x38), IC + địa chỉ codec audio, thứ tự QSPI D0–D3, TX/RX UART0.
- (Tùy chọn) Đổi prefix `iot-` → tên sản phẩm và `dev.` → tên miền lệnh.

Tìm tất cả: `grep -rn "TODO(team)" .claude/skills CLAUDE.md`

---

## 8. Tài liệu tham chiếu

- `iot-skills/README.md` — chi tiết bộ skill + cách cài/điền
- `iot-skills/CLAUDE.md` — bản đồ mặt + định tuyến skill cho agent
- `iot-skills/iot-fw-module/references/board-es3c35p.md` — **pin-map + thông số mạch đầy đủ**
- `iot-skills/iot-e2e-module/references/command-contract.md` — hợp đồng lệnh 3 chiều
- Spec gốc: LCDWIKI "3.5 inch IPS ESP32-S3 Specification" V1.0 (SKU ES3C35P)
