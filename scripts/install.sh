#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
	echo "error: install requires root (sudo $0)" >&2
	exit 1
fi

cd "${ROOT_DIR}"
make install
modprobe kernelmind 2>/dev/null || insmod "${ROOT_DIR}/kernel/kernelmind.ko"
echo "KernelMind installed and loaded."
