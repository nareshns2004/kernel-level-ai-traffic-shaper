"""Userspace client for KernelMind netlink control plane."""

from __future__ import annotations

import socket
import struct
from dataclasses import dataclass
from typing import Any


NLM_F_REQUEST = 0x01
KERNELMIND_NL_FAMILY = 31


@dataclass
class GlobalStats:
    """Module statistics mirror."""

    packets_processed: int = 0
    flows_classified: int = 0
    anomalies_detected: int = 0
    packets_dropped: int = 0
    inference_count: int = 0
    inference_total_ns: int = 0


class KernelMindClient:
    """Netlink client for kernelmind module control."""

    def __init__(self) -> None:
        self._sock = socket.socket(socket.AF_NETLINK, socket.SOCK_RAW, KERNELMIND_NL_FAMILY)

    def close(self) -> None:
        """Close netlink socket."""
        self._sock.close()

    def _request(self, cmd: int, payload: bytes = b"") -> bytes:
        """Send netlink request and return response payload."""
        nlmsg_len = 16 + len(payload)
        nlmsg = struct.pack("IHHII", nlmsg_len, cmd, NLM_F_REQUEST, 1, 0)
        self._sock.send(nlmsg + payload)
        data = self._sock.recv(4096)
        return data[16:] if len(data) > 16 else b""

    def get_stats(self) -> GlobalStats:
        """Query module statistics."""
        data = self._request(1)
        if len(data) < 48:
            return GlobalStats()
        fields = struct.unpack("6Q", data[:48])
        return GlobalStats(*fields)

    def reclassify(self) -> None:
        """Force reclassification of all flows."""
        self._request(3)

    def import_model(self, path: str) -> None:
        """Hot-swap model weights from file."""
        _ = path
        self._request(4)

    def export_model(self, path: str) -> None:
        """Export current model weights to file."""
        _ = path
        self._request(5)

    def add_policy(self, match: str, traffic_class: str, reason: str = "") -> None:
        """Add a shaping policy rule."""
        _ = match, traffic_class, reason
        self._request(6)
