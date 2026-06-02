"""Admin auth — login (DB, hash) + đổi mật khẩu.

POST /api/v1/auth/login           {username, password} → { token, username }
GET  /api/v1/auth/me              (Bearer) → { username }
POST /api/v1/auth/change-password (Bearer) {old_password, new_password} → { status }
"""
from __future__ import annotations

from fastapi import APIRouter, Depends, HTTPException, Request
from pydantic import BaseModel

from ..auth import make_admin_token
from ..deps import require_admin

router = APIRouter()


class LoginRequest(BaseModel):
    username: str
    password: str


class ChangePasswordRequest(BaseModel):
    old_password: str
    new_password: str


@router.post("/login")
async def login(request: Request, body: LoginRequest):
    admins = request.app.state.admins
    if not admins.verify(body.username, body.password):
        raise HTTPException(status_code=401, detail="invalid credentials")
    return {"token": make_admin_token(body.username), "username": body.username}


@router.get("/me")
async def me(user: str = Depends(require_admin)):
    return {"username": user}


@router.post("/change-password")
async def change_password(
    request: Request, body: ChangePasswordRequest, user: str = Depends(require_admin)
):
    admins = request.app.state.admins
    if not admins.verify(user, body.old_password):
        raise HTTPException(status_code=400, detail="old password incorrect")
    if len(body.new_password) < 4:
        raise HTTPException(status_code=400, detail="password too short (min 4)")
    admins.set_password(user, body.new_password)
    return {"status": "success"}
