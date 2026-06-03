"""Firmware store — file .bin trên đĩa + manifest.json. Đủ cho Phase 1.

manifest.json: {"firmwares": [{id, version, board, file, sha256, size, uploaded}]}
"""
from __future__ import annotations

import hashlib
import json
import threading
import time
from pathlib import Path


class FirmwareStore:
    def __init__(self, firmware_dir: str):
        self.dir = Path(firmware_dir)
        self.dir.mkdir(parents=True, exist_ok=True)
        self.manifest_path = self.dir / "manifest.json"
        self._lock = threading.Lock()
        if not self.manifest_path.exists():
            self.manifest_path.write_text(json.dumps({"firmwares": []}, indent=2))

    def _load(self) -> dict:
        return json.loads(self.manifest_path.read_text() or '{"firmwares": []}')

    def _save(self, data: dict) -> None:
        self.manifest_path.write_text(json.dumps(data, indent=2))

    def list_all(self) -> list[dict]:
        return sorted(self._load()["firmwares"], key=lambda f: f.get("uploaded", 0), reverse=True)

    def latest_for_board(self, board: str | None) -> dict | None:
        items = [
            f
            for f in self._load()["firmwares"]
            if board is None or f.get("board") in (None, board)
        ]
        if not items:
            return None
        return sorted(items, key=lambda f: f.get("uploaded", 0))[-1]

    def get(self, fw_id: str) -> dict | None:
        for f in self._load()["firmwares"]:
            if f["id"] == fw_id:
                return f
        return None

    def path(self, fw_id: str) -> Path | None:
        f = self.get(fw_id)
        if not f:
            return None
        p = self.dir / f["file"]
        return p if p.exists() else None

    def delete(self, fw_id: str) -> bool:
        """Xóa firmware khỏi manifest + xóa file (nếu không entry nào khác dùng chung file)."""
        with self._lock:
            data = self._load()
            entry = next((f for f in data["firmwares"] if f["id"] == fw_id), None)
            if not entry:
                return False
            data["firmwares"] = [f for f in data["firmwares"] if f["id"] != fw_id]
            self._save(data)
            fname = entry.get("file")
            if fname and not any(f.get("file") == fname for f in data["firmwares"]):
                p = self.dir / fname
                if p.exists():
                    try:
                        p.unlink()
                    except OSError:
                        pass
            return True

    def add(self, *, fw_id: str, version: str, board: str | None, filename: str, content: bytes) -> dict:
        with self._lock:
            sha = hashlib.sha256(content).hexdigest()
            (self.dir / filename).write_bytes(content)
            entry = {
                "id": fw_id,
                "version": version,
                "board": board,
                "file": filename,
                "sha256": sha,
                "size": len(content),
                "uploaded": time.time(),
            }
            data = self._load()
            data["firmwares"] = [f for f in data["firmwares"] if f["id"] != fw_id] + [entry]
            self._save(data)
            return entry
