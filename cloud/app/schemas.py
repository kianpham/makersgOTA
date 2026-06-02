"""Pydantic schema cho request OTA/activation/register. Lenient: thiết bị có thể gửi thừa field."""
from __future__ import annotations

from pydantic import BaseModel


class Application(BaseModel):
    version: str | None = None
    model_config = {"extra": "allow"}


class Board(BaseModel):
    type: str | None = None
    name: str | None = None
    model_config = {"extra": "allow"}


class OTARequest(BaseModel):
    application: Application | None = None
    board: Board | None = None
    chip_model_name: str | None = None
    model_config = {"extra": "allow"}


class ActivateRequest(BaseModel):
    code: str


class RegisterRequest(BaseModel):
    device_id: str
    code: str
