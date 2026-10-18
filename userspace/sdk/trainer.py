"""Model trainer SDK wrapper."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "ai"))

from trainer.trainer import ModelTrainer, TrainedModel  # noqa: E402

__all__ = ["ModelTrainer", "TrainedModel"]
