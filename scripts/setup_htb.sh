#!/usr/bin/env bash
set -euo pipefail

IFACE="${1:-eth0}"
RATE="${2:-100mbit}"

if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
	echo "error: requires root" >&2
	exit 1
fi

tc qdisc add dev "${IFACE}" root handle 1: htb default 30 2>/dev/null || \
	tc qdisc replace dev "${IFACE}" root handle 1: htb default 30

tc class add dev "${IFACE}" parent 1: classid 1:1 htb rate "${RATE}" ceil "${RATE}" 2>/dev/null || true
tc class add dev "${IFACE}" parent 1:1 classid 1:10 htb rate 200mbit ceil 400mbit prio 1 2>/dev/null || true
tc class add dev "${IFACE}" parent 1:1 classid 1:20 htb rate 300mbit ceil 700mbit prio 2 2>/dev/null || true
tc class add dev "${IFACE}" parent 1:1 classid 1:30 htb rate 100mbit ceil 500mbit prio 3 2>/dev/null || true

echo "HTB qdisc configured on ${IFACE}"
