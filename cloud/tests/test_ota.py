"""Test Phase 1: luồng OTA → activation → register → ws hello, và firmware.

Chạy từ thư mục cloud/:  python -m pytest
"""
from __future__ import annotations

import os
import tempfile

# Cấu hình test PHẢI set trước khi import app (config đọc env lúc import).
_TMP = tempfile.mkdtemp(prefix="xz_test_")
os.environ["XZ_DB_PATH"] = os.path.join(_TMP, "devices.db")
os.environ["XZ_FIRMWARE_DIR"] = os.path.join(_TMP, "firmware")
os.environ["XZ_SECRET_KEY"] = "test-secret"
os.environ["XZ_PUBLIC_HOST"] = "127.0.0.1"
os.environ["XZ_PUBLIC_PORT"] = "8000"
os.environ["XZ_ADMIN_USER"] = "admin"
os.environ["XZ_ADMIN_PASSWORD"] = "admin"

from fastapi.testclient import TestClient  # noqa: E402

from app.main import app  # noqa: E402

DEVICE = "AA:BB:CC:DD:EE:01"
OTA_BODY = {"application": {"version": "1.0.0"}, "board": {"type": "esp32s3", "name": "es3c35p"}}


def auth_headers(client):
    """Header admin cho các endpoint quản lý (devices, firmware list/upload)."""
    token = client.post(
        "/api/v1/auth/login", json={"username": "admin", "password": "admin"}
    ).json()["token"]
    return {"Authorization": f"Bearer {token}"}


def test_unregistered_device_gets_activation():
    with TestClient(app) as client:
        r = client.post("/api/v1/ota", headers={"device-id": DEVICE}, json=OTA_BODY)
        assert r.status_code == 200
        data = r.json()
        assert data["websocket"] is None
        assert data["activation"]["code"].isdigit()
        assert len(data["activation"]["code"]) == 6


def test_register_then_ota_returns_ws_and_token():
    with TestClient(app) as client:
        # 1. check-in → activation
        code = client.post("/api/v1/ota", headers={"device-id": DEVICE}, json=OTA_BODY).json()[
            "activation"
        ]["code"]
        # 2. thiết bị tự kích hoạt qua /ota/activate (đường thiết bị, mở)
        rr = client.post("/api/v1/ota/activate", headers={"device-id": DEVICE}, json={"code": code})
        assert rr.status_code == 200 and rr.json()["status"] == "success"
        # 3. OTA lại → có websocket + token
        data = client.post("/api/v1/ota", headers={"device-id": DEVICE}, json=OTA_BODY).json()
        assert data["activation"] is None
        assert data["websocket"]["url"].startswith("ws://127.0.0.1:8000")
        assert data["websocket"]["token"]


def test_wrong_activation_code_is_pending():
    with TestClient(app) as client:
        dev = "AA:BB:CC:DD:EE:02"
        client.post("/api/v1/ota", headers={"device-id": dev}, json=OTA_BODY)
        rr = client.post("/api/v1/ota/activate", headers={"device-id": dev}, json={"code": "000000"})
        assert rr.status_code == 202


def test_ws_hello_with_valid_token():
    with TestClient(app) as client:
        dev = "AA:BB:CC:DD:EE:03"
        code = client.post("/api/v1/ota", headers={"device-id": dev}, json=OTA_BODY).json()[
            "activation"
        ]["code"]
        client.post("/api/v1/ota/activate", headers={"device-id": dev}, json={"code": code})
        token = client.post("/api/v1/ota", headers={"device-id": dev}, json=OTA_BODY).json()[
            "websocket"
        ]["token"]
        with client.websocket_connect(
            "/api/v1/ws", headers={"device-id": dev, "authorization": f"Bearer {token}"}
        ) as wsconn:
            hello = wsconn.receive_json()
            assert hello["type"] == "hello"
            assert hello["session_id"]
            wsconn.send_json({"type": "ping"})
            assert wsconn.receive_json()["type"] == "pong"


def test_ws_rejects_bad_token():
    with TestClient(app) as client:
        with client.websocket_connect(
            "/api/v1/ws", headers={"device-id": "AA:BB:CC:DD:EE:04", "authorization": "Bearer nope"}
        ) as wsconn:
            assert wsconn.receive_json() == {"type": "error", "message": "unauthorized"}


def test_firmware_upload_then_ota_offers_update():
    with TestClient(app) as client:
        dev = "AA:BB:CC:DD:EE:05"
        code = client.post("/api/v1/ota", headers={"device-id": dev}, json=OTA_BODY).json()[
            "activation"
        ]["code"]
        client.post("/api/v1/ota/activate", headers={"device-id": dev}, json={"code": code})
        up = client.post(
            "/api/v1/firmware/upload",
            headers=auth_headers(client),
            data={"version": "1.1.0", "board": "es3c35p"},
            files={"file": ("fw.bin", b"\x00\x01\x02firmware", "application/octet-stream")},
        )
        assert up.status_code == 200
        data = client.post("/api/v1/ota", headers={"device-id": dev}, json=OTA_BODY).json()
        assert data["firmware"]["version"] == "1.1.0"
        fw_url = data["firmware"]["url"]
        dl = client.get("/api/v1" + fw_url.split("/api/v1", 1)[1])
        assert dl.status_code == 200
        assert dl.headers["X-Checksum-SHA256"] == data["firmware"]["sha256"]


def test_ws_connection_marks_device_online():
    with TestClient(app) as client:
        dev = "AA:BB:CC:DD:EE:0A"
        code = client.post("/api/v1/ota", headers={"device-id": dev}, json=OTA_BODY).json()[
            "activation"
        ]["code"]
        client.post("/api/v1/ota/activate", headers={"device-id": dev}, json={"code": code})
        token = client.post("/api/v1/ota", headers={"device-id": dev}, json=OTA_BODY).json()[
            "websocket"
        ]["token"]
        h = auth_headers(client)

        def device_row():
            rows = client.get("/api/v1/devices", headers=h).json()["devices"]
            return next(d for d in rows if d["device_id"] == dev)

        assert device_row()["online"] is False
        with client.websocket_connect(
            "/api/v1/ws", headers={"device-id": dev, "authorization": f"Bearer {token}"}
        ) as wsconn:
            wsconn.receive_json()  # hello
            assert device_row()["online"] is True
        assert device_row()["online"] is False


def test_firmware_list_and_devices_list():
    with TestClient(app) as client:
        h = auth_headers(client)
        # firmware list trả về (có thể rỗng hoặc có bản đã upload từ test trước trong cùng tmp)
        fl = client.get("/api/v1/firmware", headers=h)
        assert fl.status_code == 200 and "firmwares" in fl.json()
        # devices list
        dl = client.get("/api/v1/devices", headers=h)
        assert dl.status_code == 200 and "devices" in dl.json()
