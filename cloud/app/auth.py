"""Token thiết bị: HMAC-SHA256 ký gọn (payload.signature, base64url).

Phase 1 tự ký thay cho JWT lib — đủ cho xác thực thiết bị↔ws. Đổi sang JWT/khoá bất đối xứng sau nếu cần.
"""
from __future__ import annotations

import base64
import hashlib
import hmac
import json
import os
import time

from .config import settings


# ---- Password hashing (PBKDF2-SHA256, stdlib) ----


def hash_password(password: str, *, iterations: int = 120_000) -> str:
    salt = os.urandom(16)
    dk = hashlib.pbkdf2_hmac("sha256", password.encode(), salt, iterations)
    return f"pbkdf2_sha256${iterations}${base64.b64encode(salt).decode()}${base64.b64encode(dk).decode()}"


def verify_password(password: str, stored: str) -> bool:
    try:
        _algo, iters, salt_b64, hash_b64 = stored.split("$")
        salt = base64.b64decode(salt_b64)
        expected = base64.b64decode(hash_b64)
        dk = hashlib.pbkdf2_hmac("sha256", password.encode(), salt, int(iters))
    except Exception:
        return False
    return hmac.compare_digest(dk, expected)


def _b64e(b: bytes) -> str:
    return base64.urlsafe_b64encode(b).rstrip(b"=").decode()


def _b64d(s: str) -> bytes:
    return base64.urlsafe_b64decode(s + "=" * (-len(s) % 4))


def _sign(payload_b64: str, secret: str) -> str:
    sig = hmac.new(secret.encode(), payload_b64.encode(), hashlib.sha256).digest()
    return _b64e(sig)


def make_token(device_id: str, ttl_s: int | None = None, secret: str | None = None) -> str:
    secret = secret or settings.secret_key
    ttl = settings.token_ttl_s if ttl_s is None else ttl_s
    payload = {"d": device_id, "exp": int(time.time()) + ttl}
    payload_b64 = _b64e(json.dumps(payload, separators=(",", ":")).encode())
    return f"{payload_b64}.{_sign(payload_b64, secret)}"


def verify_token(token: str, secret: str | None = None) -> str | None:
    """Trả device_id nếu token hợp lệ + chưa hết hạn, ngược lại None."""
    secret = secret or settings.secret_key
    try:
        payload_b64, sig = token.split(".", 1)
    except ValueError:
        return None
    if not hmac.compare_digest(sig, _sign(payload_b64, secret)):
        return None
    try:
        payload = json.loads(_b64d(payload_b64))
    except Exception:
        return None
    if int(payload.get("exp", 0)) < int(time.time()):
        return None
    return payload.get("d")


# ---- Admin token (console quản lý) ----


def make_admin_token(username: str, ttl_s: int | None = None, secret: str | None = None) -> str:
    secret = secret or settings.secret_key
    ttl = settings.token_ttl_s if ttl_s is None else ttl_s
    payload = {"sub": username, "role": "admin", "exp": int(time.time()) + ttl}
    payload_b64 = _b64e(json.dumps(payload, separators=(",", ":")).encode())
    return f"{payload_b64}.{_sign(payload_b64, secret)}"


def verify_admin_token(token: str, secret: str | None = None) -> str | None:
    """Trả username nếu token admin hợp lệ (role=admin, chưa hết hạn), ngược lại None."""
    secret = secret or settings.secret_key
    try:
        payload_b64, sig = token.split(".", 1)
    except ValueError:
        return None
    if not hmac.compare_digest(sig, _sign(payload_b64, secret)):
        return None
    try:
        payload = json.loads(_b64d(payload_b64))
    except Exception:
        return None
    if payload.get("role") != "admin":
        return None
    if int(payload.get("exp", 0)) < int(time.time()):
        return None
    return payload.get("sub")
