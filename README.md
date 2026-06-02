# Bộ skill IoT 3 mặt — starter kit

Khung skill cho sản phẩm IoT có **ba mặt** team thường xuyên thêm code:

| Mặt | Skill | Vai trò |
|---|---|---|
| Firmware thiết bị | `iot-fw-module` | Driver/peripheral, command handler, telemetry, lớp comms |
| Backend/cloud | `iot-cloud-module` | REST endpoint, lệnh xuống thiết bị, handler sự kiện, data model |
| App điều khiển | `iot-app-module` | Màn hình, API client, action điều khiển thiết bị, realtime state |
| **(xuyên 3 mặt)** | `iot-e2e-module` | **Orchestrator** — gói cả 3 + gác hợp đồng đồng bộ 3 chiều |

Vì một tính năng điển hình đụng cả 3 mặt, `iot-e2e-module` là skill dùng nhiều nhất.

## Cài vào repo (Claude Code)

Đặt ở cấp project (skill gắn chặt repo, không để global):

```bash
cd <repo>
mkdir -p .claude/skills
cp -r iot-skills/iot-* .claude/skills/
cp iot-skills/CLAUDE.md ./CLAUDE.md        # hoặc merge vào CLAUDE.md sẵn có
```

Windows/PowerShell:

```powershell
New-Item -ItemType Directory -Force .claude\skills | Out-Null
Copy-Item -Recurse iot-skills\iot-* .claude\skills\
Copy-Item iot-skills\CLAUDE.md .\CLAUDE.md
```

Mỗi thư mục `iot-*` tạo một slash command (`/iot-fw-module`, ...). Có `userInvocable: true` nên gọi tay được, đồng thời tự bật khi mô tả tác vụ khớp `description`.

## Cách điền (3 việc còn để trống)

Tìm toàn bộ chỗ cần điền:

```bash
grep -rn "TODO(team)" .claude/skills
```

Ba nhóm cần điền cho mỗi skill:

1. **Tên & prefix.** Đổi `iot-` thành tên sản phẩm; đổi prefix lệnh `dev.` (trong `iot-e2e-module`) thành tên miền của bạn.
2. **Stack & lệnh.** Điền ngôn ngữ/framework mỗi mặt, lệnh build/test/format thật vào mục "Quy tắc bất di bất dịch" và "Build & test".
3. **Đường dẫn file mẫu.** Trỏ recipe tới file thật trong repo (giữ dạng pattern `grep`, đừng hardcode số dòng).

## Quy ước cốt lõi (đừng phá khi sửa)

- **Tham chiếu bằng pattern `grep`, không bằng số dòng** — để skill không mục khi code đổi.
- **Một mặt = một skill, nhiều recipe bên trong** — đừng tách mỗi loại việc thành một skill (routing sẽ rối).
- **Hợp đồng 3 chiều sống ở orchestrator** — tên lệnh + tên/kiểu argument phải khớp giữa App, Cloud, Firmware. Sai 1 ký tự = lệnh rớt im lặng.

## Thêm mặt mới sau này

Copy `_SKILL_TEMPLATE.md` thành `iot-<mặt>-module/SKILL.md`, điền 5 phần, rồi thêm dòng định tuyến vào `CLAUDE.md` và vào bảng "Khi nào KHÔNG dùng" của các skill khác.
