#!/usr/bin/env bash
set -euo pipefail

IFACE="${1:-lo}"
DURATION="${2:-10}"

if ! command -v iperf3 >/dev/null 2>&1; then
	echo "iperf3 not installed; skipping throughput bench"
	exit 0
fi

echo "Running ${DURATION}s throughput test on ${IFACE}..."
iperf3 -c 127.0.0.1 -t "${DURATION}" -B 127.0.0.1 2>/dev/null || \
	echo "note: start iperf3 server separately for full benchmark"
