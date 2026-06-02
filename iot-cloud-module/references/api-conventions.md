# Cloud API & frame conventions (Python / FastAPI)

Stack: **Python + FastAPI + asyncio**. Transport thiết bị: **WebSocket** (gợi ý mặc định) — TODO(team) xác nhận nếu dùng MQTT.

## Frame/giao tiếp thiết bị

- Transport: WebSocket (FastAPI `@app.websocket(...)`), một connection/thiết bị quản lý bởi `ConnectionManager`. TODO(team): xác nhận đường dẫn thật.
- Envelope dò type: JSON `{"type": "<loại>", "id": <int?>, "args"/"data": {...}}`. Dispatch theo field `type`.
- Thiết bị → cloud: telemetry/event — Pydantic model mỗi loại message. TODO(team): liệt kê các `type` thật.
- Cloud → thiết bị: lệnh điều khiển `{"id", "name": "dev.…", "args": {...}}` (xem mục lệnh) + ack/ping nếu có.

## REST/API cho app

- Base path + versioning: gợi ý `/api/v1` (`APIRouter(prefix="/api/v1")`). TODO(team): xác nhận.
- Định dạng response chuẩn: Pydantic response model; lỗi qua `HTTPException` (status + `detail`). TODO(team): chốt envelope success/error.
- Auth: gợi ý `Depends(...)` cho token/API key. TODO(team): cơ chế thật.

## Quy ước lệnh điều khiển

- Tên: `dev.<domain>.<verb>_<noun>`
- Hằng số định nghĩa tại: `CMD_<NAME>` trong module commands (`grep "CMD_"`). TODO(team): đường dẫn thật.
- Builder tại: `new_<verb>_<noun>_command(...)` trả dict/Pydantic. TODO(team): đường dẫn thật.
- Dispatch helper tại: `async def <verb>_<noun>(device_id, args)` gọi `manager.send(...)`. TODO(team): đường dẫn thật.

## Registry lệnh hiện có

| Tên lệnh | Hướng | Mô tả |
|---|---|---|
| `dev.light.set_color` (ví dụ) | cloud → fw | Đổi màu + độ sáng LED RGB |
| TODO(team) | cloud → fw | … |
