#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUTPUT="${ROOT_DIR}/ebpf/vmlinux.h"
BTF="/sys/kernel/btf/vmlinux"

if [[ ! -r "${BTF}" ]]; then
	echo "error: ${BTF} not found; need CONFIG_DEBUG_INFO_BTF=y" >&2
	exit 1
fi

if ! command -v bpftool >/dev/null 2>&1; then
	echo "error: bpftool not installed" >&2
	exit 1
fi

echo "Generating ${OUTPUT} from ${BTF}..."
bpftool btf dump file "${BTF}" format c > "${OUTPUT}"
echo "Done."
