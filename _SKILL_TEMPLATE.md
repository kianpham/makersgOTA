---
name: iot-<MẶT>-module
description: Use khi cần thêm <loại code> vào <mặt> của <sản phẩm> (<đường dẫn>/). Bao gồm <recipe 1>, <recipe 2>, <recipe 3>. Trigger khi user nói "<cụm 1>", "<cụm 2>", "<cụm 3>" hoặc gọi `/iot-<MẶT>-module`. KHÔNG dùng cho <tác vụ lệch A> — dùng <skill khác>. KHÔNG dùng khi tính năng đi nhiều mặt — dùng /iot-e2e-module.
userInvocable: true
---

# iot-<MẶT>-module — Thêm module <mặt> (<sản phẩm>)

<1-2 câu: skill làm gì, triết lý. Vd: chỉ thêm file mới, không sửa lõi.>

> Repo: `<đường dẫn>/` (<stack>).

---

## Khi nào KHÔNG dùng skill này

| Tình huống | Dùng thay |
|---|---|
| <tác vụ lệch 1> | <skill/cách khác> |
| Tính năng đi nhiều mặt | `/iot-e2e-module` |
| Fix bug / refactor file đã có | Không cần skill — sửa trực tiếp |

---

## Quy tắc bất di bất dịch

1. TODO(team): <quy ước 1 — vd không sửa abstract base, chỉ thêm file mới>.
2. TODO(team): <code style + lệnh format>.
3. TODO(team): <ràng buộc tài nguyên / an toàn nếu có>.
4. **Tham chiếu file mẫu bằng pattern `grep`, không bằng số dòng.**

---

## Recipe

### <Loại việc 1>

1. <bước>
2. <bước — kèm khối code template điền-vào-chỗ-trống>
3. Wiring: TODO(team): <grep pattern tới chỗ đăng ký/instantiate>.

---

## Build & test

```bash
# TODO(team): lệnh build, test, format thật
```

---

## Checklist trước khi báo xong

- [ ] File mới đã format theo linter
- [ ] Đăng ký/wiring đúng chỗ (kiểm bằng grep)
- [ ] Build + test xanh (báo command + output cuối)
- [ ] Không sửa lõi ngoài phạm vi cho phép
- [ ] Báo cáo: file tạo mới, dòng sửa ở file lõi, lệnh build/test, cách kiểm runtime
