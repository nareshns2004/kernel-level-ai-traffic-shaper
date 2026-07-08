"""GBDT model trainer for KernelMind."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from .dataset import TrafficDataset


class ModelTrainer:
    """Train a quantized GBDT model compatible with kernel inference."""

    def __init__(
        self,
        base_model: str = "default",
        max_depth: int = 6,
        n_estimators: int = 48,
        quantize: bool = True,
    ) -> None:
        self.base_model = base_model
        self.max_depth = max_depth
        self.n_estimators = min(n_estimators, 64)
        self.quantize = quantize

    def fit(self, dataset: TrafficDataset) -> TrainedModel:
        """Train on labeled flows (rule-based stub for bootstrap)."""
        _ = dataset
        return TrainedModel(
            version=1,
            num_trees=min(4, self.n_estimators),
            num_features=8,
            num_classes=12,
        )

    def export(self, model: TrainedModel, path: str | Path) -> None:
        """Export model to .kmmodel JSON."""
        Path(path).write_text(json.dumps(model.to_dict(), indent=2))


class TrainedModel:
    """Trained model container."""

    def __init__(
        self,
        version: int,
        num_trees: int,
        num_features: int,
        num_classes: int,
    ) -> None:
        self.version = version
        self.num_trees = num_trees
        self.num_features = num_features
        self.num_classes = num_classes

    def to_dict(self) -> dict[str, Any]:
        """Serialize to dict matching default.kmmodel format."""
        default = json.loads(
            Path(__file__).resolve().parents[1]
            .joinpath("models/default.kmmodel")
            .read_text()
        )
        default["version"] = self.version
        default["num_trees"] = self.num_trees
        default["num_features"] = self.num_features
        default["num_classes"] = self.num_classes
        return default

    def export(self, path: str | Path) -> None:
        """Write model to disk."""
        Path(path).write_text(json.dumps(self.to_dict(), indent=2))

    def verify_bpf_constraints(self) -> bool:
        """Check BPF compatibility."""
        return self.num_trees <= 64 and self.num_features <= 32
