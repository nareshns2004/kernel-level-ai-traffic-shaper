"""Dataset loading for KernelMind model training."""

from __future__ import annotations

import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Iterator

from .features import FlowFeatures, extract_from_packets


@dataclass
class LabeledFlow:
    """A labeled flow sample."""

    flow_id: str
    features: FlowFeatures
    label: str


class TrafficDataset:
    """Collection of labeled traffic flows."""

    def __init__(self, samples: list[LabeledFlow]) -> None:
        self.samples = samples

    def __len__(self) -> int:
        return len(self.samples)

    def __iter__(self) -> Iterator[LabeledFlow]:
        return iter(self.samples)

    @classmethod
    def from_csv(cls, path: str | Path) -> TrafficDataset:
        """Load dataset from CSV with columns: flow_id,label,sizes,timestamps."""
        samples: list[LabeledFlow] = []
        with open(path, newline="") as f:
            reader = csv.DictReader(f)
            for row in reader:
                sizes = [int(x) for x in row["sizes"].split(";") if x]
                ts = [float(x) for x in row["timestamps"].split(";") if x]
                feat = extract_from_packets(sizes, ts)
                samples.append(
                    LabeledFlow(
                        flow_id=row["flow_id"],
                        features=feat,
                        label=row["label"],
                    )
                )
        return cls(samples)

    @classmethod
    def from_pcap(
        cls,
        pcap_path: str | Path,
        labels: str | Path | None = None,
    ) -> TrafficDataset:
        """Load from pcap (stub — requires scapy in full install)."""
        _ = pcap_path, labels
        return cls([])
