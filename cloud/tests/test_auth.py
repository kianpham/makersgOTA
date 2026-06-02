"""Test admin auth + bảo vệ endpoint quản lý. Endpoint thiết bị vẫn mở."""
from __future__ import annotations

import os
import tempfile

_TMP = tempfile.mkdtemp(prefix="xz_auth_")
os.environ["XZ_DB_PATH"] = os.path.join(_TMP, "devices.db")
os.environ["XZ_FIRMWARE_DIR"] = os.path.join(_TMP, "firmware")
os.environ["XZ_SECRET_KEY"] = "test-secret"
os.environ["XZ_ADMIN_USER"] = "admin"
os.environ["XZ_ADMIN_PASSWORD"] = "admin"

from fastapi.testclient import TestClient  # noqa: E402

from app.main import app  # noqa: E402


def test_login_wrong_credentials():
    with TestClient(app) as client:
        r = client.post("/api/v1/auth/login", json={"username": "admin", "password": "nope"})
        assert r.status_code == 401


def test_login_ok_and_me():
    with TestClient(app) as client:
        r = client.post("/api/v1/auth/login", json={"username": "admin", "password": "admin"})
        assert r.status_code == 200
        token = r.json()["token"]
        me = client.get("/api/v1/auth/me", headers={"Authorization": f"Bearer {token}"})
        assert me.status_code == 200 and me.json()["username"] == "admin"


def test_management_requires_admin():
    with TestClient(app) as client:
        # Không token → 401
        assert client.get("/api/v1/devices").status_code == 401
        assert client.get("/api/v1/firmware").status_code == 401
        # Có token → 200
        token = client.post(
            "/api/v1/auth/login", json={"username": "admin", "password": "admin"}
        ).json()["token"]
        h = {"Authorization": f"Bearer {token}"}
        assert client.get("/api/v1/devices", headers=h).status_code == 200
        assert client.get("/api/v1/firmware", headers=h).status_code == 200


def _admin_token(client):
    return client.post(
        "/api/v1/auth/login", json={"username": "admin", "password": "admin"}
    ).json()["token"]


def test_admins_requires_token():
    with TestClient(app) as client:
        assert client.get("/api/v1/admins").status_code == 401


def test_create_admin_and_login():
    with TestClient(app) as client:
        h = {"Authorization": f"Bearer {_admin_token(client)}"}
        r = client.post("/api/v1/admins", headers=h, json={"username": "bob", "password": "bobpass"})
        assert r.status_code == 200
        # bob đăng nhập được
        lr = client.post("/api/v1/auth/login", json={"username": "bob", "password": "bobpass"})
        assert lr.status_code == 200
        # trùng username → 409
        dup = client.post("/api/v1/admins", headers=h, json={"username": "bob", "password": "x123"})
        assert dup.status_code == 409
        # list có bob
        names = [a["username"] for a in client.get("/api/v1/admins", headers=h).json()["admins"]]
        assert "bob" in names


def test_change_password():
    with TestClient(app) as client:
        h = {"Authorization": f"Bearer {_admin_token(client)}"}
        client.post("/api/v1/admins", headers=h, json={"username": "carol", "password": "old1"})
        carol = client.post("/api/v1/auth/login", json={"username": "carol", "password": "old1"}).json()["token"]
        ch = client.post(
            "/api/v1/auth/change-password",
            headers={"Authorization": f"Bearer {carol}"},
            json={"old_password": "old1", "new_password": "new1"},
        )
        assert ch.status_code == 200
        assert client.post("/api/v1/auth/login", json={"username": "carol", "password": "new1"}).status_code == 200
        assert client.post("/api/v1/auth/login", json={"username": "carol", "password": "old1"}).status_code == 401


def test_change_password_wrong_old():
    with TestClient(app) as client:
        token = _admin_token(client)
        ch = client.post(
            "/api/v1/auth/change-password",
            headers={"Authorization": f"Bearer {token}"},
            json={"old_password": "WRONG", "new_password": "whatever"},
        )
        assert ch.status_code == 400


def test_delete_admin():
    with TestClient(app) as client:
        h = {"Authorization": f"Bearer {_admin_token(client)}"}
        client.post("/api/v1/admins", headers=h, json={"username": "dave", "password": "davepass"})
        assert client.delete("/api/v1/admins/dave", headers=h).status_code == 200
        assert client.delete("/api/v1/admins/nope", headers=h).status_code == 404


def test_device_endpoints_stay_open():
    with TestClient(app) as client:
        # /ota không cần admin token (đường thiết bị)
        r = client.post(
            "/api/v1/ota",
            headers={"device-id": "AA:BB:CC:DD:EE:09"},
            json={"board": {"name": "es3c35p"}},
        )
        assert r.status_code == 200
        assert r.json()["activation"]["code"].isdigit()
