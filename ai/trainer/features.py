"""Feature extraction for traffic classification training."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Sequence


@dataclass
class FlowFeatures:
    """Per-flow feature vector for model training."""

    inter_arrival_us: float
    jitter_us: float
    avg_pkt_size: float
    entropy: float
    bytes_total: int
    packet_count: int
    tcp_flags: int

    def to_vector(self) -> list[float]:
        """Return feature vector in schema order."""
        return [
            self.inter_arrival_us,
            self.jitter_us,
            self.avg_pkt_size,
            self.entropy,
            self.bytes_total / 1024.0,
            float(self.packet_count),
            float(self.tcp_flags),
            0.0,
        ]


def extract_from_packets(
    sizes: Sequence[int],
    timestamps_us: Sequence[float],
    tcp_flags: int = 0,
) -> FlowFeatures:
    """Extract features from raw packet metadata."""
    if not sizes:
        return FlowFeatures(0, 0, 0, 0, 0, 0, tcp_flags)

    deltas = [
        timestamps_us[i] - timestamps_us[i - 1]
        for i in range(1, len(timestamps_us))
    ]
    inter = sum(deltas) / len(deltas) if deltas else 0.0
    jitter = max(deltas) if deltas else 0.0
    avg_size = sum(sizes) / len(sizes)
    entropy = sum(s & 0xFF for s in sizes) / (len(sizes) * 255.0)

    return FlowFeatures(
        inter_arrival_us=inter,
        jitter_us=jitter,
        avg_pkt_size=avg_size,
        entropy=entropy,
        bytes_total=sum(sizes),
        packet_count=len(sizes),
        tcp_flags=tcp_flags,
    )
