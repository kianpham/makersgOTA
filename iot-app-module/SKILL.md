---
name: iot-app-module
description: Use khi cần thêm module mới vào app điều khiển của <sản phẩm> (app/) — thêm màn hình/view, thêm API client gọi cloud, thêm action điều khiển thiết bị (nút bấm → lệnh xuống thiết bị), hoặc thêm subscription realtime trạng thái thiết bị. Trigger khi user nói "thêm màn hình", "thêm screen", "thêm API client", "thêm nút điều khiển", "thêm realtime state", "add app module" hoặc gọi `/iot-app-module`. KHÔNG dùng khi tính năng đi cả cloud+firmware — dùng /iot-e2e-module.
userInvocable: true
---

# iot-app-module — Thêm module app điều khiển (<sản phẩm>)

Skill này thêm code vào app điều khiển: màn hình, API client, action điều khiển thiết bị, realtime state.

> Repo: `app/` — **Flutter (Dart)**, mobile iOS+Android. State management: TODO(team) (gợi ý **Riverpod**). Lớp gọi API: HTTP client (gợi ý `dio` hoặc `http`).

---

## Khi nào KHÔNG dùng skill này

| Tình huống | Dùng thay |
|---|---|
| Tính năng đi cả cloud + firmware (lệnh điều khiển mới end-to-end) | `/iot-e2e-module` |
| Chỉ thêm endpoint phía cloud | `/iot-cloud-module` |
| Chỉ thực thi phía firmware | `/iot-fw-module` |
| Fix bug / refactor file đã có | Không cần skill — sửa trực tiếp |

---

## Bản đồ app

```
app/lib/
├── screens/      — màn hình (Widget/Page), một file một màn hình       TODO(team): xác nhận đường dẫn thật
├── api/          — API client gọi cloud (dio/http), một method/endpoint TODO(team): xác nhận đường dẫn thật
├── state/        — provider/notifier (gợi ý Riverpod)                  TODO(team): xác nhận lib + đường dẫn
└── widgets/      — Widget UI tái dùng                                  TODO(team): xác nhận đường dẫn thật
```

---

## Quy tắc bất di bất dịch

1. **Mọi gọi cloud qua lớp API client chung** (một class trong `api/`), không `dio`/`http` rải rác trong Widget.
2. **Tên action/endpoint khớp với cloud** (nếu là action điều khiển, tên lệnh do orchestrator gác).
3. **Xử lý loading + error + state lạc**: lệnh điều khiển là async; dùng `AsyncValue`/`FutureProvider` (Riverpod) phản ánh trạng thái thật từ thiết bị, không giả định thành công.
4. **Code style: `dart format` + `flutter analyze`** (không còn warning) trước khi báo xong.
5. **Tham chiếu file mẫu bằng pattern `grep`**, không số dòng.

---

## 1. Thêm màn hình / view

1. Copy màn hình gần nhất — `grep` pattern: `extends StatelessWidget`/`ConsumerWidget` hoặc khai báo route (đường dẫn thật: TODO(team)).
2. Đăng ký route (gợi ý `go_router` hoặc `Navigator`). Wiring navigation — `grep` chỗ định nghĩa routes: TODO(team).
3. Lấy data qua API client (mục 2), không gọi cloud trực tiếp.

## 2. Thêm API client gọi cloud

1. Thêm method `Future<T>` vào lớp client — `grep` pattern tên class client: TODO(team) (đường dẫn `api/`).
2. Khớp đường dẫn + schema với endpoint cloud (`/iot-cloud-module` mục 2).
3. Typing: định nghĩa model Dart cho request + response (gợi ý `json_serializable`/`freezed`), `fromJson`/`toJson` khớp field cloud.

## 3. Thêm action điều khiển thiết bị

Đây là nửa app của một lệnh điều khiển. **Nếu cloud + firmware cũng đổi, dùng `/iot-e2e-module`.**

1. UI element (nút/slider) gọi API client method tương ứng.
2. Method gọi endpoint cloud → cloud sinh lệnh xuống thiết bị.
3. UI **không** giả định thành công — phản ánh trạng thái thật khi nhận realtime (mục 4) hoặc poll lại.

## 4. Thêm subscription realtime trạng thái thiết bị

1. Đăng ký kênh realtime (WebSocket — gợi ý `web_socket_channel`) — `grep` pattern chỗ kết nối: TODO(team).
2. Map message trạng thái vào state (StreamProvider/Notifier).
3. Widget re-render theo state (`ref.watch`).

---

## Build & test

```bash
flutter pub get               # cài dependency
dart format lib               # format
flutter analyze               # lint (không còn warning)
flutter test                  # test
flutter run                   # chạy dev trên thiết bị/emulator
```

---

## Checklist trước khi báo xong

- [ ] Gọi cloud qua API client chung (không fetch rải rác)
- [ ] Đường dẫn + schema khớp endpoint cloud
- [ ] UI xử lý loading/error, không giả định lệnh thành công
- [ ] Lint + build + test xanh
- [ ] Báo cáo: file sửa, luồng UI → cloud, cách test
