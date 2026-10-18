#!/usr/bin/env python3
"""Quantize floating-point model weights to Q8.8 fixed-point."""

from __future__ import annotations


def float_to_q8_8(value: float) -> int:
    """Convert float to signed Q8.8 fixed-point."""
    scaled = int(round(value * 256))
    return max(-32768, min(32767, scaled))


def q8_8_to_float(value: int) -> float:
    """Convert Q8.8 fixed-point back to float."""
    return value / 256.0


def quantize_tree_node(node: dict) -> dict:
    """Quantize thresholds and leaf values in a tree node."""
    out = dict(node)
    if "threshold" in out:
        out["threshold_q8"] = float_to_q8_8(out.pop("threshold"))
    if "leaf_value" in out:
        out["leaf_value_q8"] = float_to_q8_8(out.pop("leaf_value"))
    return out
