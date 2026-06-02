"""WebSocket endpoint (xem PHASE1.md §4).

Auth: device-id (bắt buộc) + Bearer token (verify HMAC + khớp device-id).
Phase 1: handshake `hello` rồi giữ kết nối + ping/pong + log. Chưa xử lý audio.
"""
from __future__ import annotations

import uuid

from fastapi import APIRouter, WebSocket, WebSocketDisconnect

from ..auth import verify_token
from ..config import settings

router = APIRouter()


def _extract_auth(ws: WebSocket) -> tuple[str | None, str]:
    device_id = ws.headers.get("device-id") or ws.query_params.get("device-id")
    auth = ws.headers.get("authorization") or ws.query_params.get("authorization") or ""
    token = ""
    low = auth.lower()
    if low.startswith("bearer "):
        token = auth[7:]
    elif low.startswith("bearer+"):  # một số firmware encode khoảng trắng thành '+'
        token = auth[7:]
    token = token or ws.query_params.get("token", "")
    return device_id, token


@router.websocket("/ws")
async def ws_endpoint(ws: WebSocket):
    device_id, token = _extract_auth(ws)
    if not device_id:
        await ws.close(code=4001)
        return

    if settings.require_auth:
        tok_device = verify_token(token)
        if not tok_device or tok_device.lower() != device_id.lower():
            await ws.accept()
            await ws.send_json({"type": "error", "message": "unauthorized"})
            await ws.close(code=4003)
            return

    await ws.accept()
    manager = ws.app.state.connections
    manager.connect(device_id)
    ws.app.state.store.touch(device_id)  # cập nhật last_seen khi kết nối

    session_id = uuid.uuid4().hex
    await ws.send_json(
        {
            "type": "hello",
            "session_id": session_id,
            "audio_params": {"format": "opus", "sample_rate": 16000, "channels": 1},
        }
    )

    try:
        while True:
            msg = await ws.receive_json()
            mtype = msg.get("type")
            if mtype == "ping":
                await ws.send_json({"type": "pong"})
            # Phase 1: 'hello' từ client và các loại khác chỉ log/no-op.
    except WebSocketDisconnect:
        pass
    except Exception:
        try:
            await ws.close()
        except Exception:
            pass
    finally:
        manager.disconnect(device_id)
