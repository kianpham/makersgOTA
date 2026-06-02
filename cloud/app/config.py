"""Cấu hình server Phase 1 — đọc từ biến môi trường (prefix XZ_), có default chạy local ngay.

Phase 1 cố tình giữ nhẹ: không pydantic-settings, không DB ngoài. SQLite + file firmware.
"""
from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent  # thư mục cloud/


def _env(name: str, default: str) -> str:
    return os.environ.get(name, default)


@dataclass(frozen=True)
class Settings:
    # Bí mật ký token thiết bị — ĐỔI ở production (XZ_SECRET_KEY).
    secret_key: str = _env("XZ_SECRET_KEY", "dev-insecure-change-me")
    # Host/port công khai để dựng URL trả cho thiết bị (phải là địa chỉ thiết bị truy cập được).
    public_host: str = _env("XZ_PUBLIC_HOST", "127.0.0.1")
    public_port: int = int(_env("XZ_PUBLIC_PORT", "8000"))
    api_prefix: str = "/api/v1"
    ws_path: str = _env("XZ_WS_PATH", "/api/v1/ws")
    db_path: str = _env("XZ_DB_PATH", str(BASE_DIR / "data" / "devices.db"))
    firmware_dir: str = _env("XZ_FIRMWARE_DIR", str(BASE_DIR / "data" / "firmware"))
    token_ttl_s: int = int(_env("XZ_TOKEN_TTL_S", "86400"))
    timezone_offset_min: int = int(_env("XZ_TZ_OFFSET_MIN", "420"))  # +07:00 (Việt Nam)
    require_auth: bool = _env("XZ_REQUIRE_AUTH", "1") not in ("0", "false", "False")
    activation_ttl_ms: int = int(_env("XZ_ACTIVATION_TTL_MS", "300000"))
    # Admin console — ĐỔI ở production (XZ_ADMIN_USER / XZ_ADMIN_PASSWORD).
    admin_user: str = _env("XZ_ADMIN_USER", "admin")
    admin_password: str = _env("XZ_ADMIN_PASSWORD", "admin")


settings = Settings()


def public_base_url() -> str:
    return f"http://{settings.public_host}:{settings.public_port}"


def public_ws_url() -> str:
    return f"ws://{settings.public_host}:{settings.public_port}{settings.ws_path}"
