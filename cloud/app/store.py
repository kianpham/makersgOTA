"""Device store — SQLite (stdlib), đủ cho Phase 1.

Một thiết bị: chưa đăng ký → có mã activation; đăng ký xong → registered=1.
Gọi đồng bộ trong endpoint async (truy vấn nhanh, có lock) — đổi sang async DB nếu tải cao.
"""
from __future__ import annotations

import secrets
import sqlite3
import threading
import time
from pathlib import Path


class DeviceStore:
    def __init__(self, db_path: str):
        Path(db_path).parent.mkdir(parents=True, exist_ok=True)
        self._lock = threading.Lock()
        self._conn = sqlite3.connect(db_path, check_same_thread=False)
        self._conn.row_factory = sqlite3.Row
        self._conn.execute(
            """
            CREATE TABLE IF NOT EXISTS devices (
                device_id       TEXT PRIMARY KEY,
                registered      INTEGER NOT NULL DEFAULT 0,
                activation_code TEXT,
                activation_exp  REAL,
                board           TEXT,
                app_version     TEXT,
                created         REAL NOT NULL,
                last_seen       REAL
            )
            """
        )
        self._conn.commit()

    def get(self, device_id: str) -> sqlite3.Row | None:
        with self._lock:
            return self._conn.execute(
                "SELECT * FROM devices WHERE device_id=?", (device_id,)
            ).fetchone()

    def touch(self, device_id: str, board: str | None = None, app_version: str | None = None) -> None:
        now = time.time()
        with self._lock:
            self._conn.execute(
                """
                INSERT INTO devices (device_id, created, last_seen, board, app_version)
                VALUES (?, ?, ?, ?, ?)
                ON CONFLICT(device_id) DO UPDATE SET
                    last_seen   = excluded.last_seen,
                    board       = COALESCE(excluded.board, devices.board),
                    app_version = COALESCE(excluded.app_version, devices.app_version)
                """,
                (device_id, now, now, board, app_version),
            )
            self._conn.commit()

    def is_registered(self, device_id: str) -> bool:
        row = self.get(device_id)
        return bool(row and row["registered"])

    def ensure_activation(self, device_id: str, ttl_ms: int) -> str:
        """Trả mã activation hiện hành cho thiết bị chưa đăng ký (sinh mới nếu thiếu/hết hạn).

        Yêu cầu thiết bị đã được touch() trước (đã có row).
        """
        now = time.time()
        with self._lock:
            row = self._conn.execute(
                "SELECT activation_code, activation_exp, registered FROM devices WHERE device_id=?",
                (device_id,),
            ).fetchone()
            if row and row["registered"]:
                return ""
            code = row["activation_code"] if row else None
            exp = row["activation_exp"] if row else None
            if not code or not exp or exp < now:
                code = f"{secrets.randbelow(1_000_000):06d}"
                exp = now + ttl_ms / 1000.0
                self._conn.execute(
                    "UPDATE devices SET activation_code=?, activation_exp=? WHERE device_id=?",
                    (code, exp, device_id),
                )
                self._conn.commit()
            return code

    def register_with_code(self, device_id: str, code: str) -> str:
        """'success' | 'pending' | 'notfound'."""
        now = time.time()
        with self._lock:
            row = self._conn.execute(
                "SELECT * FROM devices WHERE device_id=?", (device_id,)
            ).fetchone()
            if not row:
                return "notfound"
            if row["registered"]:
                return "success"
            if not row["activation_code"] or row["activation_exp"] < now:
                return "pending"
            if code != row["activation_code"]:
                return "pending"
            self._conn.execute(
                "UPDATE devices SET registered=1, activation_code=NULL, activation_exp=NULL WHERE device_id=?",
                (device_id,),
            )
            self._conn.commit()
            return "success"

    def list_all(self) -> list[dict]:
        with self._lock:
            return [
                dict(r)
                for r in self._conn.execute(
                    "SELECT * FROM devices ORDER BY created DESC"
                ).fetchall()
            ]
