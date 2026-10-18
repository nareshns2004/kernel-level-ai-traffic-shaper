#!/usr/bin/env python3
"""Validate KernelMind model files against BPF constraints."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


MAX_DEPTH = 8
MAX_ESTIMATORS = 64
MAX_FEATURES = 32


def load_json(path: Path) -> dict[str, Any]:
    """Load a JSON file."""
    return json.loads(path.read_text())


def verify_model(model: dict[str, Any], schema: dict[str, Any]) -> list[str]:
    """Return a list of constraint violations."""
    errors: list[str] = []

    if model.get("version") != schema.get("version"):
        errors.append("model version mismatch with schema")

    num_trees = model.get("num_trees", len(model.get("trees", [])))
    if num_trees > MAX_ESTIMATORS:
        errors.append(f"too many trees: {num_trees} > {MAX_ESTIMATORS}")

    num_features = model.get("num_features", 0)
    if num_features > MAX_FEATURES:
        errors.append(f"too many features: {num_features} > {MAX_FEATURES}")

    for i, tree in enumerate(model.get("trees", [])):
        nodes = tree.get("nodes", [])
        if len(nodes) > MAX_DEPTH:
            errors.append(f"tree {i} depth {len(nodes)} > {MAX_DEPTH}")

        for j, node in enumerate(nodes):
            if not node.get("is_leaf"):
                for field in ("feature_idx", "threshold_q8", "left", "right"):
                    if field not in node:
                        errors.append(f"tree {i} node {j} missing {field}")
            elif "leaf_value_q8" not in node:
                errors.append(f"tree {i} leaf {j} missing leaf_value_q8")

    return errors


def main() -> int:
    """CLI entry point."""
    parser = argparse.ArgumentParser(description="Verify KernelMind model")
    parser.add_argument("--model", required=True, type=Path)
    parser.add_argument("--schema", required=True, type=Path)
    args = parser.parse_args()

    model = load_json(args.model)
    schema = load_json(args.schema)
    errors = verify_model(model, schema)

    if errors:
        for err in errors:
            print(f"VIOLATION: {err}", file=sys.stderr)
        return 1

    print(f"ok: {args.model}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
