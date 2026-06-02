"""Quản lý nhiều tài khoản admin (đều cần admin token — router include với require_admin).

GET    /api/v1/admins           → { admins: [{username, created}] }
POST   /api/v1/admins           {username, password} → tạo
DELETE /api/v1/admins/{username} → xoá (không xoá admin cuối)
"""
from __future__ import annotations

from fastapi import APIRouter, HTTPException, Request
from pydantic import BaseModel

router = APIRouter()


class CreateAdminRequest(BaseModel):
    username: str
    password: str


@router.api_route("", methods=["GET"])
@router.api_route("/", methods=["GET"])
async def list_admins(request: Request):
    return {"admins": request.app.state.admins.list()}


@router.api_route("", methods=["POST"])
@router.api_route("/", methods=["POST"])
async def create_admin(request: Request, body: CreateAdminRequest):
    if len(body.password) < 4:
        raise HTTPException(status_code=400, detail="password too short (min 4)")
    if not body.username.strip():
        raise HTTPException(status_code=400, detail="username required")
    if not request.app.state.admins.create(body.username.strip(), body.password):
        raise HTTPException(status_code=409, detail="username already exists")
    return {"status": "success", "username": body.username.strip()}


@router.delete("/{username}")
async def delete_admin(request: Request, username: str):
    result = request.app.state.admins.delete(username)
    if result == "last":
        raise HTTPException(status_code=400, detail="cannot delete the last admin")
    if result == "notfound":
        raise HTTPException(status_code=404, detail="admin not found")
    return {"status": "success"}
