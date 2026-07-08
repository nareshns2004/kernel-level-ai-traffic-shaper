#!/usr/bin/env python3
"""Compile a KernelMind GBDT model to BPF-compatible representation."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


def load_model(path: Path) -> dict[str, Any]:
    """Load model JSON."""
    return json.loads(path.read_text())


def model_to_bpf_nodes(model: dict[str, Any]) -> list[dict[str, Any]]:
    """Flatten model trees into BPF node array."""
    nodes: list[dict[str, Any]] = []
    base = 0

    for tree in model.get("trees", []):
        for node in tree.get("nodes", []):
            entry = {
                "feature_idx": node.get("feature_idx", 0),
                "threshold_q8": node.get("threshold_q8", 0),
                "left": node.get("left", 0) + base,
                "right": node.get("right", 0) + base,
                "leaf_value_q8": node.get("leaf_value_q8", 0),
                "is_leaf": 1 if node.get("is_leaf") else 0,
            }
            nodes.append(entry)
        base += len(tree.get("nodes", []))

    return nodes


def main() -> int:
    """CLI entry point."""
    parser = argparse.ArgumentParser(description="Compile GBDT to BPF")
    parser.add_argument("--model", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    model = load_model(args.model)
    nodes = model_to_bpf_nodes(model)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({"nodes": nodes}, indent=2))
    print(f"compiled {len(nodes)} nodes to {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
