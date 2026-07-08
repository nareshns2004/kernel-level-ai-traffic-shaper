"""Pytest configuration for KernelMind integration tests."""

from __future__ import annotations

import pytest


@pytest.fixture
def sample_flow_features() -> dict[str, float]:
    """Sample feature vector for testing."""
    return {
        "inter_arrival_us": 1200.0,
        "jitter_us": 300.0,
        "avg_pkt_size": 512.0,
        "entropy": 0.45,
        "bytes_kb": 64.0,
        "packet_count": 16.0,
        "tcp_flags": 0x18,
    }
