"""FastAPI dependencies dùng chung."""
from __future__ import annotations

from fastapi import Header, HTTPException

from .auth import verify_admin_token


def require_admin(authorization: str | None = Header(default=None)) -> str:
    """Yêu cầu Authorization: Bearer <admin token>. Trả username, hoặc 401."""
    token = ""
    if authorization and authorization.lower().startswith("bearer "):
        token = authorization[7:]
    user = verify_admin_token(token)
    if not user:
        raise HTTPException(status_code=401, detail="unauthorized")
    return user
