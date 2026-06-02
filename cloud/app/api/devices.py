"""Device registry — Phase 1 đăng ký mã thủ công (thay cho app/console sau).

POST /api/v1/devices/register {device_id, code}  → đăng ký thiết bị bằng mã activation
GET  /api/v1/devices                              → liệt kê (debug)
"""
from __future__ import annotations

from fastapi import APIRouter, Request, Response

from ..schemas import RegisterRequest

router = APIRouter()


@router.post("/register")
async def register(request: Request, body: RegisterRequest):
    store = request.app.state.store
    result = store.register_with_code(body.device_id, body.code)
    if result == "success":
        return {"status": "success", "device_id": body.device_id}
    if result == "pending":
        return Response(
            status_code=202, media_type="application/json", content='{"status":"pending"}'
        )
    return Response(status_code=404, media_type="application/json", content='{"status":"not_found"}')


@router.api_route("", methods=["GET"])
@router.api_route("/", methods=["GET"])
async def list_devices(request: Request):
    conns = request.app.state.connections
    devices = request.app.state.store.list_all()
    for d in devices:
        d["online"] = conns.is_online(d["device_id"])
    return {"devices": devices}
