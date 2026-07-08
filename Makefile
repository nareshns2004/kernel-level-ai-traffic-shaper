# KernelMind Traffic Shaper — top-level build orchestration
#
# Targets:
#   all          kernel module + eBPF programs + userspace tools
#   kernel       out-of-tree kernel module (kernelmind.ko)
#   ebpf         compile BPF object files
#   userspace    CLI (kernelmind) and daemon (kmshaperd)
#   ai           compile default model and verify BPF constraints
#   install      install module, binaries, and config (requires root)
#   uninstall    remove installed artifacts
#   test         run userspace and integration tests
#   kunit        build and run kernel unit tests (requires root)
#   test-config  validate config templates parse cleanly
#   vmlinux      regenerate ebpf/vmlinux.h from the running kernel
#   load         load kernelmind.ko
#   unload       unload kernelmind.ko
#   clean        remove build artifacts
#   help         show this help

SHELL := /bin/bash

ROOT_DIR    := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD_DIR   := $(ROOT_DIR)/build
KERNEL_DIR  := $(ROOT_DIR)/kernel
EBPF_DIR    := $(ROOT_DIR)/ebpf
USERSPACE   := $(ROOT_DIR)/userspace
AI_DIR      := $(ROOT_DIR)/ai
CONFIG_DIR  := $(ROOT_DIR)/config
SCRIPTS_DIR := $(ROOT_DIR)/scripts

KERNELRELEASE ?= $(shell uname -r)
KDIR          ?= /lib/modules/$(KERNELRELEASE)/build
ARCH_RAW      := $(shell uname -m)
ARCH          := $(shell uname -m | sed 's/x86_64/x86/; s/aarch64/arm64/')

CC            ?= gcc
CLANG         ?= clang
LLVM_STRIP    ?= llvm-strip
PYTHON        ?= python3
PYTEST        ?= pytest

PREFIX        ?= /usr/local
DESTDIR       ?=
MODULE_DIR    := $(DESTDIR)/lib/modules/$(KERNELRELEASE)/extra
BINDIR        := $(DESTDIR)$(PREFIX)/bin
SBINDIR       := $(DESTDIR)$(PREFIX)/sbin
SYSCONFDIR    := $(DESTDIR)/etc/kernelmind
SYSTEMD_DIR   := $(DESTDIR)/lib/systemd/system

KERNEL_MODULE := $(KERNEL_DIR)/kernelmind.ko
CLI_BIN       := $(BUILD_DIR)/bin/kernelmind
DAEMON_BIN    := $(BUILD_DIR)/sbin/kmshaperd
BENCH_BIN     := $(BUILD_DIR)/bin/bench_inference

KERNEL_SRCS := $(wildcard $(KERNEL_DIR)/*.c)
EBPF_SRCS   := $(wildcard $(EBPF_DIR)/*.bpf.c)
EBPF_OBJS   := $(patsubst $(EBPF_DIR)/%.bpf.c,$(BUILD_DIR)/ebpf/%.bpf.o,$(EBPF_SRCS))

CLI_SRCS := \
	$(USERSPACE)/cli/main.c \
	$(USERSPACE)/cli/cmd_flows.c \
	$(USERSPACE)/cli/cmd_model.c \
	$(USERSPACE)/cli/cmd_policy.c \
	$(USERSPACE)/cli/cmd_stats.c

DAEMON_SRCS := \
	$(USERSPACE)/daemon/kmshaperd.c \
	$(USERSPACE)/daemon/metrics.c \
	$(USERSPACE)/daemon/rest_api.c

CLI_OBJS    := $(patsubst $(USERSPACE)/%.c,$(BUILD_DIR)/userspace/%.o,$(CLI_SRCS))
DAEMON_OBJS := $(patsubst $(USERSPACE)/%.c,$(BUILD_DIR)/userspace/%.o,$(DAEMON_SRCS))

USERSPACE_CFLAGS  := -O2 -Wall -Wextra -Werror -I$(KERNEL_DIR) -I$(EBPF_DIR) \
                     -I$(USERSPACE)/cli -I$(USERSPACE)/daemon
USERSPACE_LDFLAGS := -lnl-3 -lnl-genl-3 -pthread

BPF_CFLAGS := -g -O2 -target bpf \
	-D__TARGET_ARCH_$(ARCH) \
	-I$(EBPF_DIR) \
	-Wall -Werror

.PHONY: all kernel ebpf userspace ai install uninstall test kunit test-config \
        test-userspace test-integration vmlinux load unload clean help bench

.DEFAULT_GOAL := all

all: kernel ebpf userspace ai

help:
	@sed -n 's/^#   /  /p' $(firstword $(MAKEFILE_LIST))

# ---------------------------------------------------------------------------
# Kernel module
# ---------------------------------------------------------------------------

kernel: $(KERNEL_MODULE)

$(KERNEL_MODULE): $(KERNEL_SRCS) $(KERNEL_DIR)/Kbuild $(KERNEL_DIR)/Makefile
	@test -d "$(KDIR)" || { \
		echo "error: kernel headers not found at $(KDIR)" >&2; \
		echo "install linux-headers-$(KERNELRELEASE) and retry" >&2; \
		exit 1; \
	}
	$(MAKE) -C $(KERNEL_DIR) modules

# ---------------------------------------------------------------------------
# eBPF programs
# ---------------------------------------------------------------------------

ebpf: $(EBPF_OBJS)

$(BUILD_DIR)/ebpf/%.bpf.o: $(EBPF_DIR)/%.bpf.c $(EBPF_DIR)/common.h $(EBPF_DIR)/vmlinux.h
	@mkdir -p $(dir $@)
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@
	$(LLVM_STRIP) -g $@

# ---------------------------------------------------------------------------
# Userspace tools
# ---------------------------------------------------------------------------

userspace: $(CLI_BIN) $(DAEMON_BIN)

$(CLI_BIN): $(CLI_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(USERSPACE_CFLAGS) $^ -o $@ $(USERSPACE_LDFLAGS)

$(DAEMON_BIN): $(DAEMON_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(USERSPACE_CFLAGS) $^ -o $@ $(USERSPACE_LDFLAGS)

$(BUILD_DIR)/userspace/%.o: $(USERSPACE)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(USERSPACE_CFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# AI / model pipeline
# ---------------------------------------------------------------------------

ai: ebpf
	@if [ -f "$(AI_DIR)/compiler/verify_constraints.py" ]; then \
		$(PYTHON) $(AI_DIR)/compiler/verify_constraints.py \
			--model "$(AI_DIR)/models/default.kmmodel" \
			--schema "$(AI_DIR)/models/model_schema.json"; \
	fi
	@if [ -f "$(AI_DIR)/compiler/gbdt_to_bpf.py" ]; then \
		$(PYTHON) $(AI_DIR)/compiler/gbdt_to_bpf.py \
			--model "$(AI_DIR)/models/default.kmmodel" \
			--output "$(BUILD_DIR)/ebpf/inference_from_model.bpf.o"; \
	fi

# ---------------------------------------------------------------------------
# Install / uninstall
# ---------------------------------------------------------------------------

install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root (sudo make install)" >&2; exit 1; }
	install -D -m 644 "$(KERNEL_MODULE)" "$(MODULE_DIR)/kernelmind.ko"
	depmod -a "$(KERNELRELEASE)"
	install -D -m 755 "$(CLI_BIN)" "$(BINDIR)/kernelmind"
	install -D -m 755 "$(DAEMON_BIN)" "$(SBINDIR)/kmshaperd"
	install -D -m 644 "$(USERSPACE)/daemon/kmshaperd.service" "$(SYSTEMD_DIR)/kmshaperd.service"
	install -D -m 644 "$(CONFIG_DIR)/kernelmind.conf.example" "$(SYSCONFDIR)/kernelmind.conf"
	install -D -m 644 "$(CONFIG_DIR)/modules-load.conf" "$(DESTDIR)/etc/modules-load.d/kernelmind.conf"
	install -D -m 644 "$(AI_DIR)/models/default.kmmodel" "$(SYSCONFDIR)/models/default.kmmodel"
	@mkdir -p "$(SYSCONFDIR)/models"
	@for obj in $(EBPF_OBJS); do \
		install -D -m 644 "$$obj" "$(SYSCONFDIR)/ebpf/$$(basename "$$obj")"; \
	done
	systemctl daemon-reload || true

uninstall:
	@test "$$(id -u)" -eq 0 || { echo "uninstall requires root (sudo make uninstall)" >&2; exit 1; }
	-$(MAKE) unload
	rm -f "$(MODULE_DIR)/kernelmind.ko"
	depmod -a "$(KERNELRELEASE)"
	rm -f "$(BINDIR)/kernelmind" "$(SBINDIR)/kmshaperd"
	rm -f "$(SYSTEMD_DIR)/kmshaperd.service"
	rm -rf "$(SYSCONFDIR)"
	rm -f "$(DESTDIR)/etc/modules-load.d/kernelmind.conf"
	systemctl daemon-reload || true

load:
	@test "$$(id -u)" -eq 0 || { echo "load requires root (sudo make load)" >&2; exit 1; }
	modprobe -r kernelmind 2>/dev/null || true
	insmod "$(KERNEL_MODULE)"

unload:
	@test "$$(id -u)" -eq 0 || { echo "unload requires root (sudo make unload)" >&2; exit 1; }
	modprobe -r kernelmind 2>/dev/null || rmmod kernelmind 2>/dev/null || true

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

test: test-config test-userspace test-integration
	@if [ -w /sys/kernel/debug/kunit 2>/dev/null ] || [ "$$(id -u)" -eq 0 ]; then \
		$(MAKE) kunit; \
	else \
		echo "skipping kunit (run 'sudo make kunit' separately)"; \
	fi

kunit:
	@test "$$(id -u)" -eq 0 || { echo "kunit requires root (sudo make kunit)" >&2; exit 1; }
	$(MAKE) -C $(KDIR) M=$(ROOT_DIR)/tests/kunit modules
	@echo "KUnit modules built in tests/kunit/ — run via your kernel's KUnit runner"

test-userspace:
	@if [ -d "$(USERSPACE)/sdk" ]; then \
		cd "$(ROOT_DIR)" && $(PYTEST) userspace/sdk/ -v; \
	else \
		echo "no userspace SDK tests found; skipping"; \
	fi

test-integration:
	cd "$(ROOT_DIR)" && $(PYTEST) tests/integration/ -v

test-config:
	@set -e; \
	for cfg in "$(CONFIG_DIR)/kernelmind.conf" "$(CONFIG_DIR)/kernelmind.conf.example"; do \
		if [ ! -s "$$cfg" ]; then \
			echo "skip empty or missing config: $$cfg"; \
			continue; \
		fi; \
		$(PYTHON) -c "import pathlib, sys; \
import importlib; \
tomllib = importlib.import_module('tomllib') if importlib.util.find_spec('tomllib') else importlib.import_module('tomli'); \
p = pathlib.Path(sys.argv[1]); \
tomllib.loads(p.read_text()); \
print('ok: {}'.format(p))" "$$cfg"; \
	done

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------

bench: $(BENCH_BIN)
	$(BENCH_BIN)

$(BENCH_BIN): tests/bench/bench_inference.c
	@mkdir -p $(dir $@)
	$(CC) -O2 -Wall -Wextra -o $@ $< -lm

# ---------------------------------------------------------------------------
# Maintenance
# ---------------------------------------------------------------------------

vmlinux:
	@$(SCRIPTS_DIR)/gen_vmlinux.sh

clean:
	$(MAKE) -C $(KERNEL_DIR) clean
	rm -rf "$(BUILD_DIR)"
	find tests/kunit -name '*.ko' -o -name '*.o' -o -name '*.mod*' -o -name '.*.cmd' | xargs -r rm -f
