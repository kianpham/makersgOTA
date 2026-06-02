"""OTA endpoint — hợp đồng kết nối lõi (xem PHASE1.md §1, §2).

GET/POST /api/v1/ota          → cấu hình server (ws+token, firmware) hoặc activation nếu chưa đăng ký
POST     /api/v1/ota/activate → thiết bị gửi mã kích hoạt
"""
from __future__ import annotations

import time

from fastapi import APIRouter, Request, Response

from ..auth import make_token
from ..config import public_base_url, public_ws_url, settings
from ..schemas import ActivateRequest, OTARequest

router = APIRouter()


def _server_time() -> dict:
    return {"timestamp": int(time.time() * 1000), "timezone_offset": settings.timezone_offset_min}


def _audio_params() -> dict:
    return {"format": "opus", "sample_rate": 16000, "channels": 1, "buffer_size_ms": 500}


async def _read_ota_body(request: Request) -> OTARequest:
    if request.method != "POST":
        return OTARequest()
    try:
        data = await request.json()
    except Exception:
        data = {}
    try:
        return OTARequest.model_validate(data or {})
    except Exception:
        return OTARequest()


def _device_id(request: Request) -> str | None:
    return request.headers.get("device-id") or request.query_params.get("device-id")


@router.api_route("", methods=["GET", "POST"])
@router.api_route("/", methods=["GET", "POST"])
async def ota_check(request: Request):
    store = request.app.state.store
    fw = request.app.state.firmware

    device_id = _device_id(request)
    if not device_id:
        return Response(
            status_code=400, media_type="application/json", content='{"error":"missing device-id"}'
        )

    body = await _read_ota_body(request)
    board = (
        (body.board.name if body.board else None)
        or (body.board.type if body.board else None)
        or body.chip_model_name
    )
    app_version = body.application.version if body.application else None
    store.touch(device_id, board=board, app_version=app_version)

    resp: dict = {"server_time": _server_time()}

    if not store.is_registered(device_id):
        code = store.ensure_activation(device_id, settings.activation_ttl_ms)
        resp["activation"] = {
            "code": code,
            "message": f"Nhập mã {code} để kích hoạt thiết bị",
            "timeout_ms": settings.activation_ttl_ms,
        }
        resp["websocket"] = None
        resp["firmware"] = None
        return resp

    resp["websocket"] = {"url": public_ws_url(), "token": make_token(device_id)}
    resp["audio_params"] = _audio_params()
    latest = fw.latest_for_board(board)
    if latest:
        resp["firmware"] = {
            "version": latest["version"],
            "url": f"{public_base_url()}{settings.api_prefix}/firmware/{latest['id']}/download",
            "sha256": latest["sha256"],
        }
    else:
        resp["firmware"] = None
    resp["activation"] = None
    return resp


@router.post("/activate")
async def activate(request: Request, body: ActivateRequest):
    store = request.app.state.store
    device_id = _device_id(request)
    if not device_id:
        return Response(
            status_code=400, media_type="application/json", content='{"error":"missing device-id"}'
        )
    result = store.register_with_code(device_id, body.code)
    if result == "success":
        return {"status": "success"}
    if result == "pending":
        return Response(
            status_code=202, media_type="application/json", content='{"status":"pending"}'
        )
    return Response(status_code=404, media_type="application/json", content='{"status":"not_found"}')
