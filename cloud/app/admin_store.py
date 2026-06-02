"""Admin store — tài khoản quản trị lưu SQLite, mật khẩu hash PBKDF2.

Seed 1 admin từ env (XZ_ADMIN_USER/PASSWORD) nếu bảng rỗng → login mặc định vẫn chạy.
"""
from __future__ import annotations

import sqlite3
import threading
import time
from pathlib import Path

from .auth import hash_password, verify_password


class AdminStore:
    def __init__(self, db_path: str):
        Path(db_path).parent.mkdir(parents=True, exist_ok=True)
        self._lock = threading.Lock()
        self._conn = sqlite3.connect(db_path, check_same_thread=False)
        self._conn.row_factory = sqlite3.Row
        self._conn.execute(
            """
            CREATE TABLE IF NOT EXISTS admins (
                username      TEXT PRIMARY KEY,
                password_hash TEXT NOT NULL,
                created       REAL NOT NULL
            )
            """
        )
        self._conn.commit()

    def _count(self) -> int:
        return self._conn.execute("SELECT COUNT(*) AS c FROM admins").fetchone()["c"]

    def ensure_seed(self, username: str, password: str) -> None:
        with self._lock:
            if self._count() == 0:
                self._conn.execute(
                    "INSERT INTO admins (username, password_hash, created) VALUES (?, ?, ?)",
                    (username, hash_password(password), time.time()),
                )
                self._conn.commit()

    def verify(self, username: str, password: str) -> bool:
        with self._lock:
            row = self._conn.execute(
                "SELECT password_hash FROM admins WHERE username=?", (username,)
            ).fetchone()
        return bool(row) and verify_password(password, row["password_hash"])

    def create(self, username: str, password: str) -> bool:
        """False nếu username đã tồn tại."""
        with self._lock:
            if self._conn.execute("SELECT 1 FROM admins WHERE username=?", (username,)).fetchone():
                return False
            self._conn.execute(
                "INSERT INTO admins (username, password_hash, created) VALUES (?, ?, ?)",
                (username, hash_password(password), time.time()),
            )
            self._conn.commit()
            return True

    def set_password(self, username: str, password: str) -> bool:
        with self._lock:
            cur = self._conn.execute(
                "UPDATE admins SET password_hash=? WHERE username=?",
                (hash_password(password), username),
            )
            self._conn.commit()
            return cur.rowcount > 0

    def delete(self, username: str) -> str:
        """'ok' | 'last' (không xoá admin cuối) | 'notfound'."""
        with self._lock:
            if self._count() <= 1:
                return "last"
            cur = self._conn.execute("DELETE FROM admins WHERE username=?", (username,))
            self._conn.commit()
            return "ok" if cur.rowcount > 0 else "notfound"

    def list(self) -> list[dict]:
        with self._lock:
            return [
                {"username": r["username"], "created": r["created"]}
                for r in self._conn.execute(
                    "SELECT username, created FROM admins ORDER BY created"
                ).fetchall()
            ]
