"""FastAPI app — Phase 1: OTA + activation + firmware + WebSocket.

Chạy: uvicorn app.main:app --host 0.0.0.0 --port 8000   (từ thư mục cloud/)
"""
from __future__ import annotations

from contextlib import asynccontextmanager

from fastapi import Depends, FastAPI
from fastapi.middleware.cors import CORSMiddleware

from .admin_store import AdminStore
from .api import admins, auth, devices, firmware, ota, ws
from .config import settings
from .connections import ConnectionManager
from .deps import require_admin
from .firmware_store import FirmwareStore
from .store import DeviceStore


@asynccontextmanager
async def lifespan(app: FastAPI):
    app.state.store = DeviceStore(settings.db_path)
    app.state.firmware = FirmwareStore(settings.firmware_dir)
    app.state.connections = ConnectionManager()
    app.state.admins = AdminStore(settings.db_path)
    app.state.admins.ensure_seed(settings.admin_user, settings.admin_password)
    yield


app = FastAPI(title="MakerSG Phase 1 — OTA + WebSocket", version="0.1.0", lifespan=lifespan)

# CORS — cho phép app web control panel gọi API (Phase 1 mở rộng; siết origin ở production).
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

_p = settings.api_prefix
# Device-facing (mở): ota, ws, firmware download.
app.include_router(ota.router, prefix=f"{_p}/ota", tags=["ota"])
app.include_router(ws.router, prefix=_p, tags=["ws"])  # → /api/v1/ws
app.include_router(firmware.router, prefix=f"{_p}/firmware", tags=["firmware"])
# Admin auth + console quản lý (devices/admins yêu cầu admin token; firmware list/upload tự bảo vệ bên trong).
app.include_router(auth.router, prefix=f"{_p}/auth", tags=["auth"])
app.include_router(
    devices.router,
    prefix=f"{_p}/devices",
    tags=["devices"],
    dependencies=[Depends(require_admin)],
)
app.include_router(
    admins.router,
    prefix=f"{_p}/admins",
    tags=["admins"],
    dependencies=[Depends(require_admin)],
)


@app.get("/")
async def root():
    return {"service": "makersg-phase1", "ota": f"{_p}/ota", "ws": settings.ws_path}


@app.get("/healthz")
async def healthz():
    return {"status": "ok"}
