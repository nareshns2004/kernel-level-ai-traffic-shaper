#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODULE="${ROOT_DIR}/kernel/kernelmind.ko"
KEY="${SIGNING_KEY:-/signing_key.pem}"
CERT="${SIGNING_CERT:-/signing_cert.pem}"

if [[ ! -f "${MODULE}" ]]; then
	echo "error: module not built: ${MODULE}" >&2
	exit 1
fi

if [[ ! -f "${KEY}" || ! -f "${CERT}" ]]; then
	echo "warning: signing keys not found; skipping module signing" >&2
	echo "unsigned module: ${MODULE}"
	exit 0
fi

/usr/src/linux-headers-"$(uname -r)"/scripts/sign-file sha256 \
	"${KEY}" "${CERT}" "${MODULE}"
echo "Signed ${MODULE}"
