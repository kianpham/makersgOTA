"""Theo dõi thiết bị đang kết nối WebSocket (online). Ref-count theo device_id (chuẩn hoá UPPER)."""
from __future__ import annotations

import threading


class ConnectionManager:
    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._online: dict[str, int] = {}  # device_id.upper() -> số kết nối

    @staticmethod
    def _k(device_id: str | None) -> str:
        return (device_id or "").upper()

    def connect(self, device_id: str) -> None:
        k = self._k(device_id)
        with self._lock:
            self._online[k] = self._online.get(k, 0) + 1

    def disconnect(self, device_id: str) -> None:
        k = self._k(device_id)
        with self._lock:
            n = self._online.get(k, 0) - 1
            if n <= 0:
                self._online.pop(k, None)
            else:
                self._online[k] = n

    def is_online(self, device_id: str) -> bool:
        with self._lock:
            return self._k(device_id) in self._online

    def count(self) -> int:
        with self._lock:
            return len(self._online)
