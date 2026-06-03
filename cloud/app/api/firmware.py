"""Firmware download/upload (xem PHASE1.md §3).

GET  /api/v1/firmware/{id}/download  → .bin (header X-Checksum-SHA256)
POST /api/v1/firmware/upload         → nạp bản mới (dev only): multipart file + version + board
"""
from __future__ import annotations

import uuid

from fastapi import APIRouter, Depends, Form, Request, Response, UploadFile
from fastapi.responses import FileResponse

from ..deps import require_admin

router = APIRouter()


@router.api_route("", methods=["GET"])
@router.api_route("/", methods=["GET"])
async def list_firmware(request: Request, _admin: str = Depends(require_admin)):
    return {"firmwares": request.app.state.firmware.list_all()}


@router.get("/{fw_id}/download")
async def download(request: Request, fw_id: str):
    fw = request.app.state.firmware
    entry = fw.get(fw_id)
    path = fw.path(fw_id)
    if not entry or not path:
        return Response(status_code=404, media_type="application/json", content='{"error":"not_found"}')
    return FileResponse(
        path,
        media_type="application/octet-stream",
        filename=entry["file"],
        headers={"X-Checksum-SHA256": entry["sha256"]},
    )


@router.delete("/{fw_id}")
async def delete_firmware(request: Request, fw_id: str, _admin: str = Depends(require_admin)):
    fw = request.app.state.firmware
    if fw.delete(fw_id):
        return {"status": "ok", "deleted": fw_id}
    return Response(status_code=404, media_type="application/json", content='{"error":"not_found"}')


@router.post("/upload")
async def upload(
    request: Request,
    file: UploadFile,
    version: str = Form(...),
    board: str | None = Form(None),
    _admin: str = Depends(require_admin),
):
    fw = request.app.state.firmware
    content = await file.read()
    entry = fw.add(
        fw_id=uuid.uuid4().hex[:12],
        version=version,
        board=board,
        filename=file.filename or "firmware.bin",
        content=content,
    )
    return {"status": "ok", "firmware": entry}
