"""Model evaluation utilities."""

from __future__ import annotations

from dataclasses import dataclass

from .dataset import TrafficDataset


@dataclass
class EvalMetrics:
    """Classification evaluation metrics."""

    accuracy: float
    f1_macro: float
    per_class_f1: dict[str, float]


def evaluate(dataset: TrafficDataset, predictions: dict[str, str]) -> EvalMetrics:
    """Compute accuracy and F1 from predictions."""
    if not len(dataset):
        return EvalMetrics(0.0, 0.0, {})

    correct = 0
    per_class: dict[str, list[bool]] = {}

    for sample in dataset:
        pred = predictions.get(sample.flow_id, "unknown")
        match = pred == sample.label
        if match:
            correct += 1
        per_class.setdefault(sample.label, []).append(match)

    per_class_f1 = {
        cls: sum(vals) / len(vals) for cls, vals in per_class.items()
    }
    f1_macro = sum(per_class_f1.values()) / len(per_class_f1) if per_class_f1 else 0.0

    return EvalMetrics(
        accuracy=correct / len(dataset),
        f1_macro=f1_macro,
        per_class_f1=per_class_f1,
    )
