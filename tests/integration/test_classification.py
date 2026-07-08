"""Integration tests for traffic classification."""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
MODEL = ROOT / "ai/models/default.kmmodel"
SCHEMA = ROOT / "ai/models/model_schema.json"
VERIFY = ROOT / "ai/compiler/verify_constraints.py"


def test_default_model_validates() -> None:
    """Default model must pass BPF constraint verification."""
    result = subprocess.run(
        [sys.executable, str(VERIFY), "--model", str(MODEL), "--schema", str(SCHEMA)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr


def test_model_schema_classes() -> None:
    """Schema must define 12 flow classes."""
    schema = json.loads(SCHEMA.read_text())
    assert len(schema["classes"]) == 12


def test_model_tree_depth(sample_flow_features: dict[str, float]) -> None:
    """Model trees must not exceed max depth."""
    model = json.loads(MODEL.read_text())
    max_depth = 8
    for tree in model["trees"]:
        assert len(tree["nodes"]) <= max_depth


@pytest.mark.parametrize(
    "label",
    ["gaming", "voip", "bulk", "streaming"],
)
def test_class_labels_exist(label: str) -> None:
    """Common class labels must be in schema."""
    schema = json.loads(SCHEMA.read_text())
    assert label in schema["classes"]
