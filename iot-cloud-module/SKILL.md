---
name: iot-cloud-module
description: Use khi cần thêm module mới vào backend/cloud của <sản phẩm> (cloud/) liên quan điều khiển và quản lý thiết bị — thêm lệnh xuống thiết bị, thêm REST/API endpoint, thêm handler nhận event từ thiết bị, hoặc thêm data model/migration. Trigger khi user nói "thêm endpoint", "thêm lệnh xuống thiết bị", "server gọi xuống thiết bị", "thêm handler event thiết bị", "thêm model", "add cloud module" hoặc gọi `/iot-cloud-module`. KHÔNG dùng khi tính năng đi cả firmware+app — dùng /iot-e2e-module.
userInvocable: true
---

# iot-cloud-module — Thêm module cloud (<sản phẩm>)

Skill này thêm code vào backend/cloud: lệnh điều khiển xuống thiết bị, endpoint cho app, handler nhận event từ thiết bị, data model.

> Repo: `cloud/` — **Python + FastAPI + asyncio**. Giao tiếp thiết bị: **WebSocket** (gợi ý mặc định, FastAPI hỗ trợ native) — TODO(team) xác nhận nếu dùng MQTT broker. DB: TODO(team) (gợi ý PostgreSQL + SQLAlchemy async, hoặc SQLite cho dev).

---

## Khi nào KHÔNG dùng skill này

| Tình huống | Dùng thay |
|---|---|
| Tính năng đi cả firmware + app (lệnh điều khiển mới end-to-end) | `/iot-e2e-module` |
| Chỉ thực thi lệnh phía firmware | `/iot-fw-module` |
| Chỉ UI/API client phía app | `/iot-app-module` |
| Fix bug / refactor file đã có | Không cần skill — sửa trực tiếp |

---

## Bản đồ cloud

```
cloud/
├── transport/   — WebSocket endpoint + ConnectionManager (1 connection/thiết bị)   TODO(team): đường dẫn thật
├── api/         — FastAPI router (APIRouter) REST cho app                          TODO(team): đường dẫn thật
├── models/      — Pydantic schema (wire) + ORM model + migration (Alembic)          TODO(team): đường dẫn thật
└── commands/    — builder (thuần data, trả Pydantic/dict) + dispatch xuống thiết bị TODO(team): đường dẫn thật
```

Quy tắc lớp: **builder lệnh** (thuần data) tách khỏi **dispatch** (gửi đi) tách khỏi **handler** (nhận về).

---

## Quy tắc bất di bất dịch

1. **ID lệnh sinh an toàn**: dùng `itertools.count()` chung hoặc counter có `asyncio.Lock`, không hardcode. (Single event loop nên thread-safe, nhưng vẫn gom một chỗ.)
2. **Mọi gửi đi qua một lớp chung** (`ConnectionManager.send(device_id, cmd)` dùng `asyncio.Queue`/lock per-connection), đừng `await ws.send_json()` rải rác.
3. **Builder thuần data** — không I/O, không log, chỉ trả Pydantic model / dict.
4. **Tên lệnh là hằng số** định nghĩa một chỗ (`CMD_<NAME> = "dev.<domain>.<verb>_<noun>"`, hoặc `StrEnum`). Khớp tuyệt đối với firmware — sai 1 ký tự = lệnh rớt im lặng.
5. **Error path không sập service** — bọc dispatch trong try/except, `logger.warning` + best-effort, đừng đóng WebSocket của thiết bị vì 1 lệnh fail.
6. **Format/lint + test**: `ruff check`, `black .`, `pytest` trước khi báo xong.
7. **Tham chiếu file mẫu bằng pattern `grep`**, không số dòng.

---

## 1. Thêm lệnh xuống thiết bị (cloud → firmware)

Đây là nửa cloud của một lệnh điều khiển. **Nếu firmware + app cũng đổi, dùng `/iot-e2e-module`.**

1. Hằng số tên lệnh — gom cùng các hằng số khác. `grep` pattern: TODO(team).
2. Builder (thuần data) tạo lệnh với argument đúng schema. File mẫu — `grep`: TODO(team).
3. Helper dispatch ngắn: sinh ID + gọi lớp gửi chung. File mẫu — `grep`: TODO(team).

```python
# Khung Python/FastAPI:
CMD_SET_COLOR = "dev.<domain>.<verb>_<noun>"     # hằng số một chỗ

class SetColorArgs(BaseModel):                    # builder thuần data (Pydantic)
    rgb: str
    brightness: int = 80

def new_set_color_command(cmd_id: int, args: SetColorArgs) -> dict:
    return {"id": cmd_id, "name": CMD_SET_COLOR, "args": args.model_dump()}

async def set_color(device_id: str, args: SetColorArgs) -> None:   # dispatch
    args.brightness = max(0, min(100, args.brightness))           # clamp defensive
    await manager.send(device_id, new_set_color_command(next_id(), args))
```

## 2. Thêm REST / API endpoint (cho app)

1. Copy router gần nhất — `grep` pattern: `@router.` hoặc `APIRouter(` (đường dẫn thật: TODO(team)).
2. Khai báo request/response bằng Pydantic model, validate tự động; gọi service/DB (async), trả response chuẩn.
3. TODO(team): auth/permission (gợi ý FastAPI `Depends(...)` cho token/API key) theo convention dự án.

## 3. Thêm handler nhận event từ thiết bị

1. Define Pydantic model cho message theo schema chung — khớp với firmware (mục telemetry của `/iot-fw-module`).
2. Thêm nhánh vào dispatcher theo field `type` của envelope — `grep` pattern handler: TODO(team) (đường dẫn thật trong `transport/`).
3. Xử lý: lưu DB / broadcast realtime cho app (qua WebSocket) / kích hoạt logic.

## 4. Thêm data model + migration

1. Thêm ORM model (gợi ý SQLAlchemy) + Pydantic schema tương ứng. TODO(team): xác nhận convention (single-file? per-table?).
2. Migration: gợi ý **Alembic** (`alembic revision --autogenerate` → `alembic upgrade head`). TODO(team): xác nhận tự chạy hay thủ công.

---

## Build & test

```bash
ruff check .            # lint
black .                 # format
pytest                  # test (builder + endpoint)
uvicorn app.main:app --reload    # chạy local — TODO(team): xác nhận entrypoint thật + DB up nếu cần
```

### Unit test builder
Builder thuần data dễ test nhất — gọi `new_*_command(...)` rồi assert tên lệnh + từng argument khớp schema. Không cần DB/network.

---

## Checklist trước khi báo xong

- [ ] Tên lệnh là hằng số một chỗ (không rải string)
- [ ] Builder + dispatch helper cùng style file mẫu
- [ ] Lint + build + test xanh
- [ ] Nếu firmware đã có lệnh tương ứng: dán 2 chuỗi tên lệnh cạnh nhau xác nhận khớp
- [ ] Báo cáo: file sửa, ví dụ payload đi xuống thiết bị, cách test

---

## Tham khảo

- [references/api-conventions.md](references/api-conventions.md) — đặc tả frame/endpoint (TODO(team) điền)
