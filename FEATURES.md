# MakerSG — Mô tả tính năng (trợ lý giọng nói, board ES3C35P)

> Sản phẩm: **trợ lý giọng nói** chạy trên board LCDWIKI ES3C35P (ESP32-S3 + màn cảm ứng 3.5" + mic + loa + codec + LED RGB + thẻ SD + pin).
> Kiến trúc 3 mặt: **App** (Flutter) điều khiển & cấu hình ↔ **Cloud** (Python/FastAPI) xử lý giọng nói & trí tuệ ↔ **Firmware** (ESP-IDF) thu/phát âm thanh & hiển thị.
> Tài liệu này mô tả *tính năng cho người dùng*. Hợp đồng lệnh kỹ thuật 3 chiều nằm ở [iot-e2e-module/references/command-contract.md](iot-e2e-module/references/command-contract.md). Pin-map: [iot-fw-module/references/board-es3c35p.md](iot-fw-module/references/board-es3c35p.md).

**Ký hiệu luồng:** `Người dùng` · `App` · `Cloud` · `FW` (firmware). Mũi tên `→` = gọi/gửi sang; `⇢` = realtime/telemetry ngược về.

---

## 1. Tóm tắt sản phẩm

Một thiết bị để bàn nghe lệnh giọng nói, gửi lên cloud để hiểu ý định và sinh câu trả lời (STT → NLU/LLM → TTS), rồi phát lại bằng loa kèm hiển thị trực quan trên màn. LED RGB báo trạng thái. App đồng hành dùng để ghép nối, cấu hình, xem lịch sử hội thoại và điều khiển từ xa.

**Ba trụ cột trải nghiệm:**
1. **Nghe – hiểu – trả lời** bằng giọng nói (vòng lặp hội thoại chính).
2. **Phản hồi trực quan** trên màn cảm ứng + LED trạng thái.
3. **Đồng hành qua app**: ghép nối, cấu hình, lịch sử, điều khiển từ xa.

---

## 2. Persona & User stories

### Persona

| Persona | Bối cảnh | Kỳ vọng chính |
|---|---|---|
| **P1 — Chủ nhà bận rộn** | Để thiết bị ở bếp/phòng khách, hay bận tay | Điều khiển rảnh tay, hẹn giờ, nhắc việc, trả lời nhanh |
| **P2 — Người dùng phổ thông** | Dùng như loa thông minh cá nhân | Hỏi đáp tự do, thời tiết, nhạc/âm báo, dễ cấu hình qua app |
| **P3 — Người cần hỗ trợ rảnh tay/thị giác** | Ưu tiên giọng nói + phản hồi âm thanh rõ | Phản hồi âm thanh đầy đủ, đèn/âm báo trạng thái rõ ràng, ít thao tác chạm |

### User stories (gom theo cụm tính năng)

**Cụm hội thoại (F1–F4)**
- Là **P1**, tôi muốn **gọi wake word khi đang bận tay** để **ra lệnh mà không cần chạm**.
- Là **P2**, tôi muốn **thấy câu hỏi của mình hiện thành chữ** để **biết thiết bị nghe đúng**.
- Là **P3**, tôi muốn **nghe câu trả lời rõ ràng qua loa** để **không phải nhìn màn**.

**Cụm phản hồi trực quan (F5–F6)**
- Là **P3**, tôi muốn **nhìn màu đèn biết thiết bị đang nghe hay đang nói** để **biết khi nào tới lượt mình**.
- Là **P2**, tôi muốn **chạm màn để chỉnh nhanh âm lượng/độ sáng** để **không phải mở app**.

**Cụm đồng hành qua app (F7–F10)**
- Là **P2**, tôi muốn **ghép thiết bị vào tài khoản qua app** để **dùng được ngay sau khi mở hộp**.
- Là **P1**, tôi muốn **đổi ngôn ngữ/giọng/độ nhạy wake trong app** để **cá nhân hóa thiết bị**.
- Là **P2**, tôi muốn **xem lại lịch sử hội thoại** để **tra lại điều đã hỏi**.

**Cụm chủ động & vận hành (F11–F13)**
- Là **P1**, tôi muốn **được nhắc khi hẹn giờ xong** để **không bỏ lỡ việc**.
- Là **P2**, tôi muốn **biết khi pin yếu** để **cắm sạc kịp**.
- Là **P3**, tôi muốn **thiết bị vẫn báo trạng thái rõ khi mất mạng** để **không bối rối**.

---

## 3. Năng lực phần cứng → tính năng

| Phần cứng (ES3C35P) | Pin chính | Tính năng nó cho phép |
|---|---|---|
| Mic MEMS (I2S DIN) | IO16 | Thu giọng nói, phát hiện wake word |
| Loa + PA + codec (I2S) | IO15, IO1, IO17/18/21 | Phát câu trả lời (TTS), âm báo, nhạc |
| Màn ST77922 QSPI 320×480 | IO9–IO14, IO41(BL) | Hiển thị transcript, câu trả lời, trạng thái, animation |
| Cảm ứng điện dung (I2C 0x38) | IO38/IO39, INT IO47 | Chạm để nói, nút bấm ảo, cuộn lịch sử |
| LED RGB địa chỉ hóa | IO40 | Báo trạng thái (idle/nghe/nghĩ/nói/lỗi) |
| Thẻ MicroSD (SDIO) | IO4/5/6/7/2/3 | Cache audio offline, log, tài nguyên giọng/âm báo |
| ADC đo pin | IO8 | Báo % pin, cảnh báo pin yếu |
| WiFi 2.4G / BLE | (tích hợp) | Kết nối cloud; BLE cho ghép nối ban đầu |

> Ràng buộc xuyên suốt: **một bus I2C dùng chung** IO38/39 (touch + codec); buffer audio/ảnh lớn (>4KB) xin từ **PSRAM** (`MALLOC_CAP_SPIRAM`); **backlight IO41 phải kéo cao** mới sáng; GPIO trống mở rộng chỉ còn **IO45/IO46**.

---

## 4. Danh sách tính năng

Mỗi tính năng ghi: **người dùng làm gì** · vai trò **App/Cloud/FW** · **luồng chi tiết** · **diễn biến trạng thái** · **skill định tuyến**.

### F1. Đánh thức & bắt đầu nghe (wake)
- **Người dùng:** nói wake word ("Hey …") *hoặc* chạm màn/giữ nút → thiết bị bắt đầu nghe.
- **Vai trò:** FW phát hiện wake/chạm, mở mic, đổi LED; Cloud mở session; App (tùy chọn) nút "nói" từ xa.
- **Luồng chi tiết:**
  1. `FW` phát hiện wake word on-device (engine — TODO team) hoặc sự kiện chạm/nút.
  2. `FW → Cloud`: mở/đánh dấu session hội thoại (`session_id`).
  3. `FW`: bật LED "đang nghe", hiện sóng âm trên màn, mở luồng thu mic (IO16) + VAD.
  4. (Biến thể từ xa) `App → Cloud → FW`: lệnh `dev.voice.start_listen` kích hoạt nghe.
- **Diễn biến trạng thái:** `Idle → Đang nghe` (LED xanh dương thở, màn hiện sóng âm).
  - *Nhánh lỗi:* không có mạng tới cloud → vẫn nghe local nhưng báo "ngoại tuyến" (xem F13); wake giả (false trigger) → tự về `Idle` sau timeout im lặng.
- **Skill:** `/iot-e2e-module` (chạm/nút từ app) hoặc `/iot-fw-module` (chỉ wake word on-device).

### F2. Thu giọng nói & nhận dạng (STT)
- **Người dùng:** nói câu lệnh; thấy transcript hiện dần trên màn.
- **Vai trò:** FW stream audio + VAD; Cloud STT trả transcript; App (tùy chọn) mirror.
- **Luồng chi tiết:**
  1. `FW ⇢ Cloud`: stream khung audio (PCM/nén — TODO team format) theo `session_id`.
  2. `Cloud`: STT realtime, trả **transcript từng phần** rồi **transcript cuối** khi VAD báo hết câu.
  3. `Cloud ⇢ FW`: đẩy transcript để hiển thị; `Cloud ⇢ App` (nếu app mở) mirror transcript.
  4. `FW`: VAD phát hiện im lặng → đóng luồng thu, chuyển sang xử lý.
- **Diễn biến trạng thái:** `Đang nghe → Đang xử lý` khi có transcript cuối (LED chuyển vàng/cam).
  - *Nhánh lỗi:* STT timeout/không nghe rõ → câu hỏi lại "Bạn nói gì cơ?" (TTS, xem F4) rồi về `Đang nghe`.
- **Skill:** `/iot-e2e-module`.

### F3. Hiểu ý định & sinh câu trả lời (NLU / LLM)
- **Người dùng:** chờ thiết bị "suy nghĩ" (LED màu nghĩ).
- **Vai trò:** Cloud phân tích ý định + sinh trả lời (± lệnh hành động); FW hiển thị "đang xử lý".
- **Luồng chi tiết:**
  1. `Cloud`: phân tích ý định từ transcript; chọn kỹ năng (hỏi đáp, hẹn giờ, thời tiết, điều khiển nhà — phạm vi TODO team).
  2. `Cloud`: gọi LLM/skill logic → sinh **text trả lời** + (tùy chọn) **lệnh hành động** (vd `dev.notify.push`, hoặc lệnh ra thiết bị khác).
  3. `Cloud → FW`: gửi text + bắt đầu TTS (F4).
- **Diễn biến trạng thái:** giữ `Đang xử lý` (LED vàng/cam xoay, màn "Đang nghĩ…").
  - *Nhánh lỗi:* LLM/skill lỗi hoặc quá thời gian → trả câu xin lỗi mặc định + LED báo lỗi ngắn rồi về `Idle`.
- **Skill:** `/iot-cloud-module` (logic) + `/iot-e2e-module` nếu kết quả là lệnh xuống thiết bị.

### F4. Phát phản hồi giọng nói (TTS) + hiển thị
- **Người dùng:** nghe câu trả lời qua loa, đọc text trên màn.
- **Vai trò:** Cloud TTS + gửi text; FW phát loa + render; App (tùy chọn) hiện lịch sử.
- **Luồng chi tiết:**
  1. `Cloud`: TTS (provider — TODO team) → stream audio + gửi `text` trả lời.
  2. `Cloud ⇢ FW`: lệnh `dev.audio.play_tts` (stream_id + text).
  3. `FW`: bật PA (IO1), phát audio qua loa (IO15); render answer card + text trên màn; LED "đang nói".
  4. `FW`: phát xong → tắt PA, về `Idle`; `Cloud ⇢ App`: ghi vào lịch sử (F10).
- **Diễn biến trạng thái:** `Đang xử lý → Đang nói → Idle` (LED xanh lá thở khi nói).
  - *Nhánh lỗi:* mất khung audio giữa chừng → dừng sạch, hiện text đầy đủ, LED báo lỗi ngắn.
- **Skill:** `/iot-e2e-module`.

### F5. Đèn LED trạng thái
- **Người dùng:** nhìn LED biết trạng thái mà không cần đọc màn; đổi theme/độ sáng LED trong app.
- **Vai trò:** FW map state→màu; App chọn theme/độ sáng.
- **Luồng chi tiết:**
  1. `FW`: map state machine (§7) → màu/hiệu ứng LED RGB (IO40, kiểu WS2812).
  2. (Đổi theme) `App → Cloud → FW`: lệnh `dev.led.set_theme` (theme + brightness).
  3. `FW`: lưu theme (NVS) + áp dụng ngay; `FW ⇢ App`: xác nhận trạng thái mới.
- **Diễn biến trạng thái:** không đổi state máy, chỉ đổi bảng màu render.
  - *Nhánh lỗi:* theme không hợp lệ → cloud clamp/giữ theme cũ, báo lỗi nhẹ về app.
- **Skill:** `/iot-fw-module` (logic local) hoặc `/iot-e2e-module` (đổi theme từ app).

### F6. Màn hình trực quan & chạm
- **Người dùng:** xem trạng thái/transcript/câu trả lời; chạm để nói, vuốt xem lịch sử gần, chỉnh nhanh âm lượng/độ sáng.
- **Vai trò:** FW vẽ UI + xử lý touch (tự chứa, không cần cloud).
- **Luồng chi tiết:**
  1. `FW`: dựng UI trên ST77922 (backlight IO41 PWM); vòng lặp đọc touch (I2C 0x38, INT IO47).
  2. `FW`: chạm vùng "nói" → kích hoạt F1; vuốt → cuộn vài mục lịch sử cache; thanh trượt → âm lượng/độ sáng (F9).
- **Diễn biến trạng thái:** điều hướng UI cục bộ; có thể bắc cầu sang `Đang nghe` (qua F1) hoặc đổi âm lượng/độ sáng (F9).
  - *Nhánh lỗi:* touch không phản hồi → kiểm GPIO/IC touch (xem QUICKSTART mục khắc phục sự cố).
- **Skill:** `/iot-fw-module`.

### F7. Ghép nối thiết bị với tài khoản (pairing)
- **Người dùng:** mở app → quét/nhập mã → thiết bị vào WiFi & gắn vào tài khoản. (Luồng first-run đầy đủ ở §5.)
- **Vai trò:** FW chế độ ghép nối; Cloud đăng ký + cấp token; App onboarding.
- **Luồng chi tiết:**
  1. `FW`: vào chế độ ghép nối (BLE hoặc AP tạm) khi chưa có cấu hình.
  2. `App → FW` (BLE/AP): gửi SSID + mật khẩu WiFi + mã ghép nối.
  3. `FW → Cloud`: kết nối WiFi, đăng ký bằng mã ghép nối.
  4. `Cloud`: tạo bản ghi thiết bị, cấp **device token**, gắn vào tài khoản; `Cloud → App`: xác nhận.
- **Diễn biến trạng thái:** `Ghép nối (LED tím/đặc thù) → Idle` khi thành công.
  - *Nhánh lỗi:* sai mật khẩu WiFi → báo trên app + LED lỗi, quay lại bước nhập; hết thời gian ghép nối → thoát chế độ ghép nối, hướng dẫn thử lại.
- **Skill:** `/iot-e2e-module`.

### F8. Cấu hình & cá nhân hóa
- **Người dùng:** trong app chỉnh ngôn ngữ, giọng đọc, độ nhạy wake, âm lượng/độ sáng mặc định, theme LED, múi giờ. (Catalog đầy đủ ở §6.)
- **Vai trò:** App màn cài đặt; Cloud lưu + đẩy xuống FW; FW áp dụng.
- **Luồng chi tiết:**
  1. `App → Cloud`: lưu cấu hình theo thiết bị (REST).
  2. `Cloud → FW`: lệnh `dev.config.apply` cho phần ảnh hưởng thiết bị (ngôn ngữ, độ nhạy wake…) + các lệnh chuyên biệt (âm lượng/độ sáng/theme).
  3. `FW`: áp dụng + lưu NVS; `FW ⇢ App`: xác nhận trạng thái mới.
- **Diễn biến trạng thái:** không đổi state máy; thay tham số vận hành.
  - *Nhánh lỗi:* giá trị ngoài miền → cloud clamp theo §6; FW từ chối giá trị không hợp lệ + giữ giá trị cũ.
- **Skill:** `/iot-e2e-module` (ảnh hưởng thiết bị) hoặc `/iot-cloud-module` (chỉ lưu cloud).

### F9. Điều khiển nhanh tại chỗ (âm lượng / độ sáng / mute mic)
- **Người dùng:** chạm thanh trượt/nút để chỉnh âm lượng, độ sáng, **mute mic** (riêng tư).
- **Vai trò:** FW xử lý local tức thì + báo telemetry để app sync.
- **Luồng chi tiết:**
  1. `FW`: nhận chạm → đổi âm lượng codec / PWM backlight / cờ mute mic ngay lập tức.
  2. `FW ⇢ Cloud ⇢ App`: telemetry trạng thái mới (`volume`, `brightness`, `mic_muted`).
- **Diễn biến trạng thái:** mute mic → `Mute mic` (LED đỏ tĩnh, biểu tượng mic gạch chéo); bỏ mute → về state trước đó.
  - *Nhánh lỗi:* khi đang mute, wake word bị bỏ qua có chủ đích — hiển thị nhắc "mic đang tắt".
- **Skill:** `/iot-fw-module` (+ telemetry để app sync).

### F10. Lịch sử hội thoại
- **Người dùng:** xem lại câu đã hỏi & trả lời trong app (và vài mục gần nhất trên màn thiết bị).
- **Vai trò:** Cloud lưu + API; App màn hình; FW cache vài mục gần.
- **Luồng chi tiết:**
  1. `Cloud`: sau mỗi lượt (F4) lưu `{transcript, answer, timestamp}` theo session/tài khoản.
  2. `App → Cloud`: REST lấy danh sách (phân trang), tìm kiếm, xóa.
  3. `FW`: giữ cache N mục gần nhất (RAM/SD) để vuốt xem offline.
- **Diễn biến trạng thái:** không đổi state máy.
  - *Nhánh lỗi:* lưu DB lỗi → log + best-effort, không chặn vòng hội thoại.
- **Skill:** `/iot-cloud-module` (lưu/API) + `/iot-app-module` (màn hình).

### F11. Thông báo chủ động (proactive)
- **Người dùng:** nhận nhắc hẹn giờ/lời nhắc → thiết bị nói + hiện trên màn + LED nhấp nháy.
- **Vai trò:** Cloud đẩy sự kiện; FW phát + hiển thị; App đặt nhắc.
- **Luồng chi tiết:**
  1. `App → Cloud`: đặt hẹn giờ/nhắc việc (thời điểm, nội dung).
  2. `Cloud`: tới hạn → `Cloud → FW`: lệnh `dev.notify.push` (kind + text).
  3. `FW`: phát âm báo/TTS + hiện thông báo + LED nhấp nháy; chờ người dùng xác nhận (chạm) hoặc tự ẩn.
- **Diễn biến trạng thái:** `Idle/bất kỳ → Đang nói (thông báo) → trở lại state trước`.
  - *Nhánh lỗi:* thiết bị offline lúc tới hạn → cloud thử lại khi reconnect; nếu mute → ưu tiên hiển thị + LED, hạn chế âm thanh.
- **Skill:** `/iot-e2e-module`.

### F12. Pin & nguồn
- **Người dùng:** xem % pin trên màn/app; được cảnh báo khi pin yếu.
- **Vai trò:** FW đọc ADC + telemetry + cảnh báo local; Cloud/App hiển thị.
- **Luồng chi tiết:**
  1. `FW`: định kỳ đọc ADC pin (IO8) → quy ra %.
  2. `FW ⇢ Cloud ⇢ App`: telemetry `battery_level` (+ cờ đang sạc nếu có).
  3. `FW`: dưới ngưỡng → cảnh báo cục bộ (biểu tượng pin + LED/âm báo nhẹ).
- **Diễn biến trạng thái:** lớp phủ cảnh báo, không đổi state hội thoại; pin cực thấp → vào chế độ tiết kiệm/tắt an toàn.
  - *Nhánh lỗi:* đọc ADC nhiễu → lọc trung bình trượt trước khi cảnh báo.
- **Skill:** `/iot-fw-module` (telemetry) + `/iot-app-module` (hiển thị).

### F13. Offline / fallback
- **Người dùng:** khi mất mạng, thiết bị vẫn báo trạng thái rõ ràng và xử lý được vài lệnh đơn giản đã cache.
- **Vai trò:** FW dùng SD cache + báo trạng thái offline; tự reconnect.
- **Luồng chi tiết:**
  1. `FW`: mất kết nối cloud → hiện "ngoại tuyến" + LED đặc thù; tạm ngừng vòng STT/LLM.
  2. `FW`: phục vụ câu trả lời/âm báo cố định từ SD (vd "Hiện mất mạng, vui lòng thử lại").
  3. `FW → Cloud`: tự thử reconnect (backoff); khi lại được → đồng bộ telemetry tồn đọng.
- **Diễn biến trạng thái:** `Bất kỳ → Lỗi/mất mạng (LED đỏ nhấp nháy) → Idle` khi reconnect.
  - *Nhánh lỗi:* WiFi có nhưng cloud chết → phân biệt "mất WiFi" vs "mất cloud" trong thông báo.
- **Skill:** `/iot-fw-module`.

---

## 5. Luồng onboarding / first-run

Đưa thiết bị mới từ hộp tới sẵn sàng hội thoại.

```
[Bật nguồn lần đầu]
      │  chưa có cấu hình WiFi/token trong NVS
      ▼
[FW vào chế độ ghép nối]  ── LED tím · màn hiện "Mở app để ghép nối" + mã/QR
      │  BLE quảng bá (hoặc AP tạm)
      ▼
[App phát hiện & kết nối]  ── App quét BLE / chọn thiết bị
      │
      ▼
[App: chọn WiFi + nhập mật khẩu]  ── App gửi SSID/pass + mã ghép nối qua BLE/AP
      │
      ▼
[FW kết nối WiFi] ──(sai mật khẩu)──▶ [Báo lỗi trên app + LED lỗi] ──▶ quay lại nhập WiFi
      │ thành công
      ▼
[FW → Cloud: đăng ký bằng mã]  ── Cloud cấp device token, gắn vào tài khoản
      │
      ▼
[Self-test: mic · loa · màn · LED]  ── phát âm chào + animation; xác nhận phần cứng
      │
      ▼
[Hoàn tất] ── LED Idle · màn màn chờ · sẵn sàng wake word
```

**Nhánh lỗi chính:**
- **Sai mật khẩu WiFi** → app báo, FW giữ chế độ ghép nối, cho nhập lại.
- **Hết thời gian ghép nối** (không có app kết nối trong N phút) → FW thoát chế độ ghép nối, màn hướng dẫn nhấn giữ nút để vào lại.
- **Đăng ký cloud thất bại** (token/mạng) → thử lại có backoff; nếu vẫn lỗi, hiển thị mã lỗi + link hỗ trợ.

> Tham số thời gian (timeout ghép nối, số lần thử) và cơ chế truyền credential (BLE vs AP tạm): TODO team chốt.

---

## 6. Catalog cài đặt đầy đủ

| Thiết lập | Miền giá trị | Mặc định | Áp dụng tại | Lệnh liên quan |
|---|---|---|---|---|
| Ngôn ngữ | mã ngôn ngữ (vd `vi`, `en`) | `vi` | Cloud (STT/LLM/TTS) + FW (TTS) | `dev.config.apply` |
| Giọng TTS | id giọng (provider) | giọng mặc định ngôn ngữ | Cloud | (cloud, qua cấu hình) |
| Độ nhạy wake word | `int 0–100` | `60` | FW | `dev.config.apply` |
| Âm lượng | `int 0–100` | `70` | FW (codec) | `dev.audio.set_volume` |
| Độ sáng màn | `int 0–100` | `80` | FW (PWM IO41) | `dev.display.set_brightness` |
| Theme LED | id theme (vd `default`, `calm`, `vivid`) | `default` | FW (IO40) | `dev.led.set_theme` |
| Độ sáng LED | `int 0–100` | `80` | FW | `dev.led.set_theme` |
| Múi giờ | IANA tz (vd `Asia/Ho_Chi_Minh`) | theo tài khoản | Cloud + FW (hiển thị giờ) | `dev.config.apply` |
| Chế độ riêng tư (mặc định mute) | `bool` | `false` | FW | `dev.mic.set_mute` |
| Auto-sleep màn | giây (`0` = không) | `60` | FW | `dev.config.apply` |

> Quy tắc range: app chặn UX, cloud clamp defensive, FW kiểm an toàn — cả ba độc lập (xem §10 và command-contract). Danh sách giọng/theme cụ thể: TODO team.

---

## 7. Trạng thái thiết bị (state machine — LED + màn + audio)

| Trạng thái | Sự kiện kích hoạt vào | LED RGB | Màn hình | Audio |
|---|---|---|---|---|
| Idle | khởi động xong / phát xong trả lời | Trắng dịu / tắt nhẹ | Đồng hồ / màn chờ | — |
| Đang nghe | wake word / chạm "nói" / `start_listen` | Xanh dương thở | Sóng âm + transcript dần | thu mic |
| Đang xử lý | có transcript cuối (VAD hết câu) | Vàng/cam xoay | "Đang nghĩ…" | — |
| Đang nói | nhận `play_tts` / `notify.push` | Xanh lá thở | Card câu trả lời + text | TTS qua loa |
| Mute mic | `mic.set_mute(true)` / chạm mute | Đỏ tĩnh | Biểu tượng mic gạch chéo | — |
| Ghép nối | first-run / nhấn giữ nút ghép nối | Tím / đặc thù | "Mở app để ghép nối" + mã/QR | âm chào (self-test) |
| Lỗi / mất mạng | mất cloud/WiFi / lỗi xử lý | Đỏ nhấp nháy | Thông báo lỗi | âm báo ngắn |

> Map màu/hiệu ứng cụ thể đổi được từ app qua theme (F5). Bảng màu chi tiết: TODO team khi làm UI.

---

## 8. Yêu cầu phi chức năng (NFR)

| Khía cạnh | Mục tiêu (đề xuất — TODO team chốt) | Ghi chú phần cứng/triển khai |
|---|---|---|
| Độ trễ wake → bắt đầu nghe | < 300 ms | xử lý on-device; tránh chặn vòng UI |
| Độ trễ transcript từng phần | < 500 ms sau khi nói | STT streaming; phụ thuộc mạng + provider |
| Độ trễ round-trip (hết câu → bắt đầu nói) | < 1.5 s | gồm STT cuối + LLM + bắt đầu TTS stream |
| Thời lượng pin (chủ động) | TODO team (theo dung lượng pin) | LiPo 3.7V; tối ưu auto-sleep màn + light-sleep |
| Dòng chờ / deep-sleep | càng thấp càng tốt | tận dụng light/deep-sleep khi idle lâu |
| Reconnect WiFi/cloud | tự động, backoff, không cần thao tác | phân biệt mất WiFi vs mất cloud (F13) |
| Bộ nhớ buffer audio/ảnh | dùng **PSRAM** cho buffer > 4KB | `heap_caps_malloc(.., MALLOC_CAP_SPIRAM)` fallback `MALLOC_CAP_8BIT`; có 8MB OPI PSRAM |
| Bus I2C | **một bus dùng chung** IO38/39 (touch + codec) | không tạo bus thứ hai; quản nhiều device khác địa chỉ |
| Công suất loa | ≤ **1.5W@8Ω** hoặc **2W@4Ω** | không vượt để tránh méo/hỏng |
| Hiển thị | 320×480 @ tốc độ mượt cho animation cơ bản | QSPI ST77922; framebuffer trong PSRAM; backlight IO41 PWM |
| Độ tin cậy âm thanh | không pop/click khi bật/tắt PA | quản trình tự bật PA (IO1) quanh phát/dừng |

> Các số liệu là đề xuất khởi điểm để team chốt theo đo thực tế và provider được chọn.

---

## 9. Riêng tư & bảo mật

- **Chỉ thu sau wake word / chủ ý:** mic chỉ stream sau khi wake word kích hoạt hoặc người dùng chạm "nói"; không thu nền liên tục lên cloud.
- **Tín hiệu trực quan khi đang thu:** LED "đang nghe" + biểu tượng mic phải bật bất cứ khi nào đang thu — không thu ẩn.
- **Mute mic phần cứng/logic:** F9 cho phép tắt mic; khi mute, wake word bị bỏ qua có chủ đích và hiển thị rõ.
- **Chính sách lưu/xóa dữ liệu:** quy định rõ audio thô có lưu không (mặc định **không lưu audio thô**, chỉ transcript+answer cho lịch sử); cho phép người dùng xóa lịch sử (F10); thời hạn lưu: TODO team.
- **Mã hóa truyền:** mọi kết nối thiết bị↔cloud và app↔cloud qua **TLS/WSS**; không gửi credential/khoá dạng rõ.
- **Xác thực thiết bị:** mỗi thiết bị có **device token** cấp khi ghép nối (F7); cloud xác thực token trên mỗi phiên; thu hồi được.
- **Xác thực người dùng (app↔cloud):** TODO team (token/API key qua `Depends(...)` — xem api-conventions).
- **Xóa khi unpair:** gỡ thiết bị khỏi tài khoản → xóa token + dữ liệu phiên liên quan; FW về chế độ ghép nối, xóa cấu hình NVS.
- **Lưu cục bộ trên SD:** cache/log trên SD nên hạn chế dữ liệu nhạy cảm; cân nhắc mã hóa nếu lưu transcript.

---

## 10. Registry lệnh điều khiển (đầy đủ args — phạm vi FEATURES.md)

Các tính năng xuyên 3 mặt hiện thực qua lệnh `dev.<domain>.<verb>_<noun>`. Bảng dưới là **schema args đầy đủ** ở mức spec; khi thi hành phải đẩy qua `/iot-e2e-module` và **đồng bộ chính thức** sang [command-contract.md](iot-e2e-module/references/command-contract.md) + [api-conventions.md](iot-cloud-module/references/api-conventions.md).

### Lệnh cloud/app → firmware

| Lệnh | Tính năng | Args (kiểu, required/default, range) | Mô tả |
|---|---|---|---|
| `dev.voice.start_listen` | F1 | `timeout_ms: int` (default 8000, range 1000–30000) | Bắt đầu thu mic trong cửa sổ timeout |
| `dev.voice.stop_listen` | F1 | — | Dừng thu mic ngay |
| `dev.audio.play_tts` | F4 | `stream_id: str` (required); `text: str` (required) | Phát audio TTS theo stream + hiển thị text |
| `dev.audio.set_volume` | F8/F9 | `level: int` (required, range 0–100) | Đặt âm lượng loa |
| `dev.display.set_brightness` | F8/F9 | `level: int` (required, range 0–100) | Đặt độ sáng màn (PWM IO41) |
| `dev.led.set_theme` | F5 | `theme: str` (required); `brightness: int` (default 80, range 0–100) | Đổi theme + độ sáng LED RGB |
| `dev.mic.set_mute` | F9 | `muted: bool` (required) | Bật/tắt mute mic |
| `dev.notify.push` | F11 | `kind: str` (required); `text: str` (required) | Đẩy thông báo chủ động (nói + hiển thị) |
| `dev.config.apply` | F8 | `language: str` (optional); `wake_sensitivity: int` (optional, 0–100); `timezone: str` (optional); `auto_sleep_s: int` (optional, ≥0) | Áp cấu hình ảnh hưởng thiết bị (các field optional, chỉ gửi field cần đổi) |

### Telemetry firmware → cloud (⇢ app)

| Message `type` | Tính năng | Field (kiểu) | Mô tả |
|---|---|---|---|
| `battery_level` | F12 | `percent: int (0–100)`; `charging: bool` | Mức pin định kỳ |
| `device_state` | §7 | `state: str` (idle/listening/processing/speaking/muted/pairing/error) | Trạng thái máy hiện tại |
| `mic_muted` | F9 | `muted: bool` | Trạng thái mute mic |
| `volume` | F9 | `level: int (0–100)` | Âm lượng hiện tại |
| `brightness` | F9 | `level: int (0–100)` | Độ sáng màn hiện tại |
| `wake_event` | F1 | `source: str` (wakeword/touch/remote) | Một lần đánh thức |
| `error` | F13 | `code: str`; `message: str` | Lỗi/mất kết nối |

> ⚠️ Mỗi lệnh xuyên 3 mặt PHẢI đi qua `/iot-e2e-module` để khớp tuyệt đối tên lệnh + args giữa App/Cloud/Firmware (sai 1 ký tự = lệnh rớt im lặng). Range chặn 3 lớp độc lập: UX ở app, clamp ở cloud, an toàn ở firmware.

---

## 11. Còn phải chốt (TODO team)

- **Wake word engine** on-device (vd Espressif esp-sr/WakeNet, hay chỉ nút/chạm).
- **Provider STT / NLU·LLM / TTS** (tự host hay API ngoài) + danh sách giọng TTS.
- **Định dạng & transport audio** (PCM thô vs Opus; WebSocket stream vs chunk) — ảnh hưởng NFR độ trễ.
- **Phạm vi "kỹ năng" trợ lý** (thời tiết, hẹn giờ, điều khiển nhà, hỏi đáp tự do…).
- **Số liệu NFR** (§8) chốt theo đo thực tế + dung lượng pin thật.
- **Chính sách lưu/xóa audio & transcript** (§9): có lưu audio thô không, thời hạn lưu, mã hóa SD.
- **Tham số onboarding** (§5): timeout ghép nối, BLE vs AP tạm, cơ chế truyền credential.
- **Bảng màu/theme LED & danh sách theme** (§6/§7).
- Chốt schema từng lệnh (§10) rồi **đồng bộ** sang [command-contract.md](iot-e2e-module/references/command-contract.md) khi thi hành qua `/iot-e2e-module`.
