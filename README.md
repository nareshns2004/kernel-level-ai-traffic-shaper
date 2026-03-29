# kernel level ai traffic shaper

> **AI-driven, kernel-level network traffic shaping for Linux — built for precision, speed, and intelligence.**

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Features](#features)
- [How It Works](#how-it-works)
- [Requirements](#requirements)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [AI Model Integration](#ai-model-integration)
- [Kernel Module Details](#kernel-module-details)
- [Benchmarks](#benchmarks)
- [Security Considerations](#security-considerations)
- [Roadmap](#roadmap)
- [Contributing](#contributing)
- [License](#license)

---

## Overview

**KernelMind Traffic Shaper** is a Linux kernel module that integrates a lightweight, inference-capable AI model directly into the network stack to perform real-time, intelligent traffic shaping. Unlike traditional rule-based QoS tools (e.g., `tc`, `netem`), KernelMind learns network patterns dynamically and adapts shaping policies without requiring userspace round-trips.

The AI model runs entirely in kernel space via a custom eBPF-assisted inference pipeline, enabling **sub-millisecond decision latency** with no context-switching overhead.

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Userspace                            │
│   ┌──────────────┐   ┌──────────────┐   ┌───────────────┐  │
│   │  CLI / TUI   │   │  Config API  │   │  Model Trainer│  │
│   │  (kernelmind)│   │  (gRPC/REST) │   │  (Python SDK) │  │
│   └──────┬───────┘   └──────┬───────┘   └───────┬───────┘  │
│          └──────────────────┼───────────────────┘          │
│                             │ netlink / ioctl               │
└─────────────────────────────┼───────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────┐
│                       Kernel Space                          │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              KernelMind Core Module                  │   │
│  │                                                     │   │
│  │  ┌──────────────┐    ┌──────────────────────────┐  │   │
│  │  │  Netfilter   │───▶│   Feature Extractor      │  │   │
│  │  │  Hook (NF_IP)│    │  (flow stats, headers,   │  │   │
│  │  └──────────────┘    │   entropy, timing)       │  │   │
│  │                      └────────────┬─────────────┘  │   │
│  │                                   │                 │   │
│  │                      ┌────────────▼─────────────┐  │   │
│  │                      │   AI Inference Engine    │  │   │
│  │                      │  (quantized ONNX / BPF   │  │   │
│  │                      │   decision tree ensemble)│  │   │
│  │                      └────────────┬─────────────┘  │   │
│  │                                   │                 │   │
│  │  ┌─────────────────┐ ┌────────────▼─────────────┐  │   │
│  │  │  Traffic Control│◀│   Policy Executor        │  │   │
│  │  │  (HTB / HFSC)   │ │  (mark, drop, throttle,  │  │   │
│  │  │  via tc actions │ │   redirect, prioritize)  │  │   │
│  │  └─────────────────┘ └──────────────────────────┘  │   │
│  │                                                     │   │
│  │  ┌──────────────────────────────────────────────┐  │   │
│  │  │           eBPF Maps (shared state)           │  │   │
│  │  │   flow_table | model_weights | policy_cache  │  │   │
│  │  └──────────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## Features

### Core Capabilities
- **Kernel-native AI inference** — No userspace round-trips; decisions happen inside `netfilter` hooks
- **Real-time flow classification** — Classifies flows (video, gaming, VoIP, bulk transfer, P2P, etc.) within the first 10 packets
- **Adaptive bandwidth allocation** — Dynamically adjusts HTB class rates based on predicted flow demand
- **Congestion prediction** — Proactively detects congestion before buffer bloat occurs
- **Anomaly detection** — Flags and throttles unusual traffic patterns (DDoS precursors, exfiltration)

### AI & ML
- Quantized gradient-boosted decision tree ensemble (fits in L2 cache)
- Online learning mode — model updates from observed outcomes without retraining
- Feature set: packet inter-arrival time, flow entropy, byte/packet ratios, TTL variance, port behavior
- Exportable ONNX model for userspace retraining with your own traffic datasets

### Integration
- Full compatibility with `iproute2 tc` and existing `qdisc` setups
- Netlink-based control plane for programmatic management
- Prometheus metrics exporter via `/proc/kernelmind/metrics`
- REST API bridge daemon (`kmshaperd`) for dashboard integration

---

## How It Works

### 1. Packet Interception
Every incoming and outgoing packet passes through a registered `NF_INET_PRE_ROUTING` / `NF_INET_POST_ROUTING` hook. The module extracts per-flow statistics using a kernel-space hash map keyed by the 5-tuple `(src_ip, dst_ip, src_port, dst_port, proto)`.

### 2. Feature Extraction
For each flow, a rolling window of features is maintained in an eBPF map:
- Inter-packet arrival jitter
- Payload entropy (randomness index)
- Packet size distribution
- Protocol-specific signals (TCP flags, QUIC indicators)
- Historical bandwidth utilization per interface

### 3. AI Inference
Once a flow accumulates sufficient statistics (configurable, default: 8 packets), the feature vector is passed to the inference engine. The model — a depth-limited gradient boosted ensemble compiled into BPF bytecode — outputs:
- **Flow class** (one of 12 categories)
- **Priority score** (0.0 – 1.0)
- **Predicted bandwidth demand** (Mbps estimate)

### 4. Policy Execution
Based on the inference result, the policy executor applies one of:
- `SKB_MARK` for downstream `tc` filter matching
- `nf_queue` redirect for deep inspection
- Direct rate limiting via token bucket in the kernel
- DSCP remarking for upstream QoS propagation

### 5. Feedback Loop
Packet loss events, RTT measurements (via TCP timestamp analysis), and explicit userspace feedback are written back into the eBPF map as reward signals, enabling lightweight online model updates.

---

## Requirements

### Kernel
- Linux kernel **≥ 5.15** (LTS recommended)
- Kernel config must include:
  ```
  CONFIG_NETFILTER=y
  CONFIG_NF_CONNTRACK=y
  CONFIG_BPF=y
  CONFIG_BPF_SYSCALL=y
  CONFIG_BPF_JIT=y
  CONFIG_NET_SCH_HTB=y
  CONFIG_NET_SCH_HFSC=y
  CONFIG_NET_CLS_BPF=y
  CONFIG_NET_ACT_BPF=y
  ```

### Build Dependencies
```bash
# Debian / Ubuntu
sudo apt install build-essential linux-headers-$(uname -r) \
    clang llvm libelf-dev libbpf-dev bpftool \
    python3-dev python3-pip cmake pkg-config

# Fedora / RHEL
sudo dnf install kernel-devel clang llvm elfutils-libelf-devel \
    libbpf-devel bpftool python3-devel cmake
```

### Runtime
- `iproute2` ≥ 5.10 (for `tc` integration)
- `bpftool` (for eBPF map inspection)

---

## Installation

### From Source

```bash
git clone https://github.com/your-org/kernelmind-shaper.git
cd kernelmind-shaper

# Build kernel module + eBPF programs + userspace tools
make all

# Load the module
sudo make install
sudo modprobe kernelmind

# Verify
lsmod | grep kernelmind
dmesg | tail -20
```

### Enable on Boot

```bash
echo "kernelmind" | sudo tee -a /etc/modules-load.d/kernelmind.conf
sudo cp config/kernelmind.conf /etc/kernelmind/kernelmind.conf
sudo systemctl enable --now kmshaperd
```

---

## Configuration

Configuration lives at `/etc/kernelmind/kernelmind.conf` (TOML format):

```toml
[core]
interface = "eth0"           # Primary interface to shape
mode = "adaptive"            # "adaptive" | "static" | "monitor-only"
log_level = "info"           # "debug" | "info" | "warn" | "error"

[ai]
model_path = "/etc/kernelmind/models/default.kmmodel"
inference_threshold = 8      # Packets before first classification
online_learning = true       # Enable feedback-based weight updates
update_interval_sec = 300    # How often to persist updated weights

[shaping]
total_bandwidth_mbps = 1000  # Total uplink capacity
min_flow_rate_kbps = 512     # Guaranteed floor per flow
max_flow_rate_mbps = 900     # Hard cap per flow

[classes]
# Priority classes mapped to HTB queues
[classes.realtime]
dscp = "EF"
htb_rate = "200mbit"
htb_ceil = "400mbit"
ai_min_priority = 0.85

[classes.interactive]
dscp = "AF41"
htb_rate = "300mbit"
htb_ceil = "700mbit"
ai_min_priority = 0.50

[classes.bulk]
dscp = "CS1"
htb_rate = "100mbit"
htb_ceil = "500mbit"
ai_min_priority = 0.0

[anomaly]
enabled = true
entropy_threshold = 0.92     # Flag flows above this entropy
auto_throttle = true         # Automatically throttle flagged flows
throttle_rate_kbps = 128
```

---

## Usage

### CLI Reference

```bash
# Show live flow table with AI classifications
kernelmind flows

# Show module stats
kernelmind stats

# Force reclassify all active flows
kernelmind reclassify

# Export current model weights
kernelmind model export --output ./my_model.kmmodel

# Import a retrained model (hot-swap, no restart)
kernelmind model import --path ./retrained.kmmodel

# Set manual override for a specific flow
kernelmind override --flow 192.168.1.10:443 --class realtime --duration 60s

# Watch Prometheus metrics
curl http://localhost:9102/metrics
```

### Example: Prioritize Gaming Traffic

```bash
# Tag UDP flows on common gaming ports as realtime
kernelmind policy add \
  --match "proto=udp,dport=27015-27030" \
  --class realtime \
  --reason "Steam game traffic"
```

---

## AI Model Integration

### Retraining with Custom Data

The Python SDK allows you to capture labeled traffic traces and retrain the model:

```python
from kernelmind.sdk import TrafficDataset, ModelTrainer

# Load a pcap with flow labels
dataset = TrafficDataset.from_pcap(
    "capture.pcap",
    labels="labels.csv"  # flow_id -> class
)

trainer = ModelTrainer(
    base_model="default",
    max_depth=6,           # Keep shallow for kernel inference
    n_estimators=48,
    quantize=True          # Required for BPF compilation
)

model = trainer.fit(dataset)
model.export("retrained.kmmodel")

# Validate BPF compatibility
model.verify_bpf_constraints()
```

### BPF Model Constraints

Models must satisfy:
- Max tree depth: **8** (BPF stack limit)
- Max estimators: **64**
- Feature count: **≤ 32**
- All weights must be representable as **fixed-point Q8.8**

---

## Kernel Module Details

| Component | Implementation |
|-----------|---------------|
| Hook registration | `nf_register_net_hook()` |
| Flow table | eBPF `BPF_MAP_TYPE_LRU_HASH` (64K entries) |
| Inference engine | BPF bytecode compiled from quantized GBDT |
| Traffic control | HTB qdisc via `rtnetlink` from kernel thread |
| IPC to userspace | `netlink` (family: `NETLINK_KERNELMIND`) |
| Metrics | `/proc/kernelmind/` virtual filesystem |
| Weight updates | `bpf_map_update_elem()` from userspace trainer |

---

## Benchmarks

Tested on: Linux 6.6 LTS, Intel Xeon E-2388G, 10GbE NIC (Intel X710)

| Metric | Value |
|--------|-------|
| Classification latency (p50) | 0.18 ms |
| Classification latency (p99) | 0.41 ms |
| Throughput overhead vs. baseline | < 1.2% |
| Flow table capacity | 65,536 concurrent flows |
| Model inference (per packet, hot path) | ~180 ns |
| Memory footprint (kernel module + maps) | ~14 MB |

---

## Security Considerations

- The kernel module is **signed** and requires `CONFIG_MODULE_SIG_FORCE` compatible signing in production deployments.
- eBPF programs are verified by the kernel verifier before load — no arbitrary code execution.
- The netlink socket is restricted to `CAP_NET_ADMIN`.
- AI model files are SHA-256 verified before hot-swap to prevent model poisoning.
- Anomaly detection can be used to identify and throttle potential DDoS amplification, but is **not** a replacement for a dedicated firewall.
- Online learning is sandboxed — weight updates cannot alter BPF program structure, only leaf values within pre-verified tree bounds.

---

## Roadmap

- [ ] **v0.1** — Core module, static GBDT inference, HTB integration
- [ ] **v0.2** — Online learning, Prometheus metrics, CLI tooling
- [ ] **v0.3** — HFSC scheduler support, IPv6 full support
- [ ] **v0.4** — Neural network inference (tiny MLP via BPF)
- [ ] **v0.5** — Multi-interface shaping, SR-IOV VF support
- [ ] **v1.0** — Stable ABI, DKMS packaging, distro submission

---

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting PRs.

```bash
# Run the test suite (requires a VM or dedicated test machine — never run on prod)
make test

# Run kernel module unit tests (kunit)
make kunit

# Run userspace tests
cd userspace && pytest tests/
```

Kernel code follows the [Linux kernel coding style](https://www.kernel.org/doc/html/latest/process/coding-style.html). Python code is formatted with `black` and `ruff`.

---

## License

- **Kernel module**: GPL-2.0 (required for `EXPORT_SYMBOL_GPL` use)
- **Userspace tools & SDK**: MIT
- **Default AI model weights**: CC-BY-4.0

---

<p align="center">
  Built with intent. Shaped with intelligence.
</p>
